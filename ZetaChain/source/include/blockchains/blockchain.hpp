#pragma once 

/*
MIT License

Copyright (c) 2017 ZetaChain_Native

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "platform.hpp" // Platform Specific Stuff NOTE: Must Always be the first include in a file
#include <iostream> // std::cout
#include <string> // std::string
#include <algorithm> // std::find_if
#include <vector> // std::vector
#include <map> // std::map
#include <stdexcept> // throw std::runtime_error()
#include "thirdparty/json.hpp"
#include "conversions.hpp"
#include "operators.hpp"
#include "constants.hpp" // MAX_BLOCK_SIZE
#include "opencl/init.hpp"
#include "opencl/openclhandle.hpp"
#include "opencl/opencldata.hpp"
#include "opencl/programarguments.hpp"
#include "opencl/kernelarguments.hpp"
#include "opencl/commandqueuearguments.hpp"
#include "opencl/ndrangekernelarguments.hpp"
#include "opencl/opencldata.hpp"
#include "opencl/readbufferarguments.hpp"

extern bool __noOpenCL;

extern "C" void lockBlockChainASM(unsigned long timeout);

namespace ZetaChain_Native {

	template <class BlockType>
	class Blockchain {

	public:
		Blockchain(){

		}

		virtual ~Blockchain() {
			// for(int i = 0; i < this->blocks->size() - 1; i++) {
			// 	delete this->blocks[i];
			// }
			// delete this->blocks;

			// for(int i = 0; i < this->orphanedChains->size(); i++) [
			// 	delete this->orphanedChains[i];
			// ]
			// delete this->orphanedChains;
		}

		std::string computeHash() {
			return Hashing::hashVector(this->toBytes());
		}

		template <class BlockType>

		bool add(BlockType block) {
			if(block->getData() == nullptr)
				throw std::runtime_error("Can not add a null block to a blockchain");
			
			block->setHeight(this->count + 1);
			block->setIndex(this->count);

			if(lastBlock == nullptr)
				block->setPreviousHash("");
			else
				block->setPreviousHash(lastBlock->getHash());

			block->mine();
			block->lock();

			std::string hash = block->getHash();
			if(!containsBlockByHash(hash)) {
				this->blocks.insert( {hash, *block} );
				this->count++;
				lastBlock = block;
				return true;
			}
			std::cout << "[WARNING] Block with Hash: " << hash << " was already present in the blockchain Skipping" << std::endl;
			return false;
		}


		bool containsBlockByHash(std::string hash){
			if(this->blocks.find(hash) == this->blocks.end())
				return false;
			
			return true;
		}

		BlockType getBlockByHash(std::string hash){
			if(!this->containsBlockByHash(hash))
				return nullptr;

			return this->blocks[hash];
		}

		bool containsBlockByHeight(unsigned long height) {
			return height <= count + 1;
		}

		BlockType getBlockByHeight(unsigned long height) {
			if(!this->containsBlockByHeight(height))
				return nullptr;
			
			std::string hash = this->lastBlock->getHash();
			BlockType blk = *this->lastBlock;

			while(hash != ""){
				blk = getBlockByHash(hash);
				if(blk.getHeight() == height)
					return blk;
				hash = blk.getPreviousHash();
			}

			return nullptr;
		}

		std::vector<unsigned char> toBytes() {
			std::vector<unsigned char> bytes = std::vector<unsigned char>(sizeof(Blockchain<BlockType>));
			if(this->lastBlock != nullptr){
				// for(int i = 0 i < sizeof(BlockData) - 1; i++){
				// 	bytes.push_back(static_cast<unsigned char>(*(this->lastBlock + i)));
				// }
				bytes += this->lastBlock->toBytes();
			}
			for(auto it = this->blocks.begin(); it != this->blocks.end(); it++) {
				bytes += it->second->toBytes();
			}
			if (this->orphanedChains.size() > 0) {
				for (int i = 0; i < this->orphanedChains.size() - 1; i++) {
					bytes += orphanedChains[i]->toBytes();
				}
			}
			return bytes;
		}

		bool verify() {
			for(auto itr = blocks.begin(); itr != blocks.end(); itr++) {
				if(!itr->second.verify())
				return false;
			}
			return true;
		}

		bool lock(unsigned long timeout = 1000) {
			if(this->timeLocked != 0)
			return true;
		
			if(!__noOpenCL) {
				OpenCL::OpenCLLockingData* lockingData = OpenCL::OpenCLLockingData::getInstance();
				if(!lockingData->handle){
					lockingData->handle = OpenCL::init();
					if(!lockingData->handle){
						throw std::runtime_error("Failed to Initialise OpenCL Handle please update your drivers or restart the application with the --noOpenCL option");
					}
				}
				else {
					cl_program program = lockingData->handle->createProgram(lockingData->handle->loadKernel("kernels/lockblockchain.cl"));
					lockingData->currentProgram = new OpenCL::OpenCLProgram(program, "kernels/lockblockchain.cl", &lockingData->handle);
					if(!lockingData->currentProgram) {
						throw std::runtime_error("Failed to Create Program with Kernel Code kernels/lockblockchain.cl");
					}
					OpenCL::ProgramArguments pArgs = {
						program,
						lockingData->handle->getDevices().size(),
						lockingData->handle->getDevices().data(),
						nullptr,
						nullptr,
						nullptr
					};
					lockingData->handle->checkError(lockingData->handle->buildProgram(pArgs));
					cl_int error = CL_SUCCESS;
					OpenCL::KernelArguments kArgs = {
						lockingData->currentProgram->getProgram(),
						"lockblock.cl",
						&error
					};
					cl_kernel kernel = lockingData->handle->createKernel(kArgs);
					lockingData->currentKernel = new OpenCL::OpenCLKernel(kernel, "kernels/lockblockchain.cl", &lockingData->handle);
					lockingData->handle->checkError(error);
					unsigned long kernelData = timeout + 1; // Adding 1 because of OpenCL control thread
					const size_t KERNEL_DATA_SIZE = sizeof(kernelData);
					OpenCL::BufferArguments aBufferArgs = {
						CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
						KERNEL_DATA_SIZE,
						reinterpret_cast<void*>(&kernelData),
						&error
					};
					cl_mem aBuffer = lockingData->handle->createBuffer(aBufferArgs);
					lockingData->currentABuffer = new OpenCL::OpenCLBuffer<unsigned long>(aBuffer, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, kernelData, &lockingData->handle);
					int result = -1;
					OpenCL::BufferArguments bBufferArgs = {
						CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
						sizeof(int),
						reinterpret_cast<void*>(&result),
						&error
					};
					cl_mem bBuffer = lockingData->handle->createBuffer(bBufferArgs);
					lockingData->currentBBuffer = new OpenCL::OpenCLBuffer<int>(bBuffer, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, kernelData, &lockingData->handle);
					OpenCL::CommandQueueArguments commandArgs = {
						lockingData->handle->getDevices()[0],
						NULL,
						&error
					};
					cl_command_queue commandQueue = lockingData->handle->createCommandQueue(commandArgs);
					lockingData->currentCommandQueue = new OpenCL::OpenCLCommandQueue(commandQueue, lockingData->handle->getDevices()[0], NULL, &lockingData->handle);
					OpenCL::KernelArgArguments arg_iterations = {
						kernel,
						0,
						sizeof(cl_mem),
						&aBuffer
					};
					lockingData->handle->setKernelArgument(arg_iterations);
					OpenCL::KernelArgArguments arg_result = {
						kernel,
						1,
						sizeof(cl_mem),
						&bBuffer
					};
					lockingData->handle->setKernelArgument(arg_result);
					const size_t globalWorkSize[] = { KERNEL_DATA_SIZE, 0, 0 };
					OpenCL::NDRangeKernelArguments ndArgs = {
						commandQueue,
						kernel,
						1,
						nullptr,
						0,
						nullptr,
						NULL
					};
					lockingData->handle->checkError(lockingData->handle->enqueueNDRangeKernel(ndArgs));
					OpenCL::ReadBufferArguments readArgs = {
						commandQueue,
						bBuffer,
						CL_TRUE,
						0,
						sizeof(int),
						reinterpret_cast<void*>(&result),
						0,
						nullptr,
						nullptr
					};
					lockingData->handle->checkError(lockingData->handle->enqueueReadBuffer(readArgs));
					lockingData->handle->releaseCommandQueue(commandQueue);
					lockingData->handle->releaseMemObject(bBuffer);
					lockingData->handle->releaseMemObject(aBuffer);
					lockingData->handle->releaseKernel(kernel);
					lockingData->handle->releaseProgram(program);
	
					delete lockingData->currentCommandQueue;
					delete lockingData->currentBBuffer;
					delete lockingData->currentABuffer;
					delete lockingData->currentKernel;
					delete lockingData->currentProgram;
					delete lockingData->handle;
				}
			}
			else {
				lockBlockChainASM(timeout);
			}
	
			time_t now;
			time(&now);
			struct tm* timeinfo;
			timeinfo = localtime(&now);
			this->timeLocked = now - (timeout / 1000);
			time_t _time = this->timeLocked;
			this->hash = computeHash();
			std::cout << "Blockchain has Sucessfully been locked" << std::endl;
			return this->timeLocked != 0;;
		}

		std::string toString() {
			nlohmann::json j;
			nlohmann::json lBlock = this->lastBlock->toString();
			nlohmann::json blocksArr = nlohmann::json::array();
			nlohmann::json orphanedChainsArr = nlohmann::json::array();

			std::vector<std::string> blockValues = std::vector<std::string>(this->blocks.size());

			for(auto itr = this->blocks.begin(); itr != this->blocks.end(); itr++) {
				blockValues.push_back(itr->second.toString());
			}

			for(int i = 0; i < blockValues.size() - 1; i++) {
				nlohmann::json obj = blockValues[i];
				blocksArr.push_back(obj);
			}

			for(int i = 0; i < orphanedChains.size() - 1; i++) {
				nlohmann::json obj = orphanedChains[i]->toString();
				orphanedChainsArr.push_back(obj);
			}

			j["lastBlock", lBlock.dump()];
			j["count", this->count];
			j["orphanCount", this->orphanCount];
			j["blocks", blocksArr.dump()];
			j["orphanedChains", orphanedChainsArr.dump()];
			return j;
		}

		std::map<std::string, BlockType> getBlocks() {
			return this->blocks;
		}

		std::vector<Blockchain<BlockType>*> getOrphanedChains() {
			return this->orphanedChains;
		}

		BlockType* getLastBlock() {
			return this->lastBlock;
		}

		unsigned long getCount() {
			return this->count;
		}

		unsigned long getOrphanCount() {
			return this->orphanCount;
		}

		void setBlocks(std::map<std::string, BlockType> blocks) {
			if(this->blocks.size() != 0)
				throw std::runtime_error("Can not set blocks for a non empty blockchain");
			this->blocks = blocks;
		}

		void setOrphanedChains(std::vector<Blockchain<BlockType>*> orphanedChains) {
			if(this->orphanedChains.size() != 0)
				throw std::runtime_error("Can not set orphaned chains for a non empty blockchain");
			this->orphanedChains = orphanedChains;
		}

		void setLastBlock(BlockType* lastBlock) {
			if(this->lastBlock != nullptr)
				throw std::runtime_error("Can not modify last block as it is not null");
			this->lastBlock = lastBlock;
		}

		void setCount(unsigned long count) {
			if(this->count != 0)
				throw std::runtime_error("Can not set block count for a non empty blockchain");
			this->count = count;
		}

		void setOrphanCount(unsigned long orphanCount) {
			if(this->orphanCount != 0)
				throw std::runtime_error("Can not set orphan count for a non empty blockchain");
			this->count = orphanCount;
		}

	protected:

	private:
		BlockType* lastBlock = new BlockType();
		unsigned long count = 0;
		unsigned long orphanCount = 0;
		std::map<std::string, BlockType> blocks = std::map<std::string, BlockType>();
		std::vector<Blockchain<BlockType>*> orphanedChains = std::vector<Blockchain<BlockType>*>();
	};
}