# RL4Net Simulator - A simulator for reinforcement learning based networking algorithm research

Implementing a reinforcement learning environment and algorithms for networking from scratch is a difficult task. Inspired by the work of [ns3-gym](https://github.com/tkn-tub/ns3-gym), we developed RL4Net (Reinforcement Learning for Network) to facilitate the research and simulator of reinforcement learning for networking. 

Below Figure shows the architecture of RL4Net:

<p align="center">
<img src="doc/RL4Net_architecture.png" alt="drawing" width="600"/>
</p>

RL4Net is composed of two functional blocks:  

- <b>Environment</b>: Environment is built on widely used ns3 network simulator [ns3](https://www.nsnam.org/). We extend ns3 with six components:   
    - <b>Metric Extractor</b> for computing quality metrics like delay and loss from ns3;   
    - <b>Computers</b> for translating quality metrics to DRL state and reward;  
    - <b>Action Operator</b> to get action commands from agent;  
    - <b>Action Executor</b> for perform ns3 operations by actions;   
    - <b>ns3Env</b> for transforming the ns3 object into DRL environment;   
    - <b>envInterface</b> to translate between ns3 data and DRL factors.     
- <b>Agent</b>: Agent is container of a DRL-based cognitive routing algorithm. A agent can built on various deep learning frameworks like pyTorch and Tensorflow.

# Description of folders

- <b>./ns3-addon</b>: Files to be copied into ns3 source file folder for extension. It includes:
    - ns3-src/action-executor: code for Action Executor
    - ns3-src/metric-extractor: code for Metric Extractor
    - rapidjson: an open source JSON parser and generator
    - ns3-scratch: several examples of experiments on RL4Net
- <b>./ns3-env</b>: File for ns3Env block. It cinludes:
    - env-interface: code for envInterface
    - ns3-python-connector: code for connecting python and ns3 c++
- <b>./RL4Net-lib</b>: Libaray files developed by us
- <b>./TE-trainer</b>: Files for traning agents
- <b>./RLAgent</b>: Files of agents

# Contact

Jun Liu (liujun@bupt.edu.cn), Beijing University of Posts and Telecommunications, China
