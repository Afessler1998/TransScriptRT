transScriptRT implements a real-time speech analysis engine with a simple and configurable API.
It is designed to leverage multicore Intel processors and CUDA enabled Nvidia GPUs for fast and
efficient analysis of speech. Your machine must meet the requirements outlined in the following links:

*CPU Requirements*
https://www.intel.com/content/www/us/en/developer/articles/system-requirements/intel-oneapi-threading-building-blocks-system-requirements.html

*GPU Requirements*
https://docs.nvidia.com/deeplearning/tensorrt/support-matrix/index.html

*Usage*
With all features enabled, transScriptRT outputs a stream of a real time script
which labels speech with speaker's names and emotional state. You may enable or
disable various analyses for your individual use case, but there are some
restrictions to consider.#   T r a n s S c r i p t R T  
 