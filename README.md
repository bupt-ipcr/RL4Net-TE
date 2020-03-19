# RL4Net Simulator 
## - A simulator of reinforcement learning algorithm research for networking

Implementing a reinforcement learning environment and algorithms for networking from scratch is a difficult task. Inspired by the work of [ns3-gym](https://github.com/tkn-tub/ns3-gym), we developed RL4Net (Reinforcement Learning for Network) to facilitate the research and simulator of reinforcement learning for networking. 

Below Figure shows the architecture of RL4Net:

<p align="center">
<img src="doc/RL4Net_architecture.png" alt="drawing" width="600"/>
</p>

RL4Net is composed of two functional blocks:

1. <b>Environment</b>: Environment is built on widely used ns3 network simulator [ns3](https://www.nsnam.org/). We extend ns3 with six components:   
    (1) <b>Metric Extractor</b> for computing quality metrics like delay and loss from ns3;   
    (2) <b>Computers</b> for translating quality metrics to DRL state and reward;  
    (3) <b>Action Operator</b> to get action commands from agent;  
    (4) <b>Action Executor</b> for perform ns3 operations by actions;   
    (5) <b>ns3Env</b> for transforming the ns3 object into DRL environment;   
    (6) <b>envInterface</b> to translate between ns3 data and DRL factors.     
2. <b>Agent</b>: Agent is container of a DRL-based cognitive routing algorithm. A agent can built on various deep learning frameworks like pyTorch and Tensorflow.
