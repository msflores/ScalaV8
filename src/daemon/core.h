// Copyright (c) 2014-2020, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "blocks/blocks.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "misc_log_ex.h"
#include "rapidjson/document.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #include "libipfs/include/libipfs-windows.h"
#elif __APPLE__
    #include "libipfs/include/libipfs-macos.h"
#else
    #include "libipfs/include/libipfs-linux.h"
#endif

#undef SCALA_DEFAULT_LOG_CATEGORY
#define SCALA_DEFAULT_LOG_CATEGORY "daemon"

using namespace rapidjson;

namespace daemonize
{

class t_core final
{
public:
  static void init_options(boost::program_options::options_description & option_spec)
  {
    cryptonote::core::init_options(option_spec);
  }
private:
  typedef cryptonote::t_cryptonote_protocol_handler<cryptonote::core> t_protocol_raw;
  cryptonote::core m_core;
  // TEMPORARY HACK - Yes, this creates a copy, but otherwise the original
  // variable map could go out of scope before the run method is called
  boost::program_options::variables_map const m_vm_HACK;
public:
  t_core(
      boost::program_options::variables_map const & vm
    )
    : m_core{nullptr}
    , m_vm_HACK{vm}
  {
    //initialize core here
    MGINFO("Initializing core...");

    //initialize IPFS here
	  MGINFO("Initializing IPFS...");
    
    const char* IPFSstartMessage = IPFSStartNode((char*)"./");

	  Document startMessage;
	  startMessage.Parse(IPFSstartMessage);

    std::string parsedMessage = startMessage["Message"].GetString();

    if (parsedMessage.find("started on port") != std::string::npos) {
      MGINFO("Initialized new IPFS daemon...");
    }
    else if(parsedMessage.find("busy") != std::string::npos){
      MGINFO("Reusing existing running IPFS daemon...");
    }
    else{
      MGINFO("Could not initialize IPFS...");
      m_core.graceful_exit();
    }

    #if defined(PER_BLOCK_CHECKPOINT)
        const cryptonote::GetCheckpointsCallback& get_checkpoints = blocks::GetCheckpointsData;
    #else
        const cryptonote::GetCheckpointsCallback& get_checkpoints = nullptr;
    #endif
    if (!m_core.init(m_vm_HACK, nullptr, get_checkpoints))
    {
      throw std::runtime_error("Failed to initialize core");
    }
    MGINFO("Core initialized OK");
  }

  // TODO - get rid of circular dependencies in internals
  void set_protocol(t_protocol_raw & protocol)
  {
    m_core.set_cryptonote_protocol(&protocol);
  }

  bool run()
  {
    return true;
  }

  cryptonote::core & get()
  {
    return m_core;
  }

  ~t_core()
  {
    MGINFO("Deinitializing core...");
    try {
      m_core.deinit();
      m_core.set_cryptonote_protocol(nullptr);

    MGINFO("Deinitializing IPFS...");
    
    const char* IPFSstopMessage = IPFSStopNode();
    Document stopMessage;
	  stopMessage.Parse(IPFSstopMessage);
    std::string parsedMessage = stopMessage["Message"].GetString();

      if (parsedMessage.find("IPFS node stopped") != std::string::npos) {
        MGINFO("IPFS daemon stopped...");
      }
      
     } catch (...) {
      MERROR("Failed to deinitialize core...");
    }
  }
};

}
