<!--
 * @author: Jiawei Wu
 * @create time: 2020-03-19 20:58
 * @edit time: 2020-03-29 17:30
 * @FilePath: /README.md
 -->
# RL4Net Simulator - A simulator for research of Reinforcement Learning based Networking algorithm

Implementing a reinforcement learning environment and algorithms for networking from scratch is a difficult task. Inspired by the work of [ns3-gym](https://github.com/tkn-tub/ns3-gym), we developed RL4Net (<b>R</b>einforcement <b>L</b>earning for <b>Net</b>working) to facilitate the research and simulator of reinforcement learning for networking. 

Below figure shows the architecture of RL4Net:

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

# Installation  

## Install ns3  

Since RL4Net is based on ns-3, you need to install ns-3 before use RL4Net.  
The introcuction of ns-3 and how to install can be find at the [official website](https://www.nsnam.org/) of ns-3.  
As a recommendation, you can  

1. Install dependencies follow the guide.  
2. Then use git to install **ns-3-dev** (you can also install a specific version od ns-3, such as ns-3.30, but we prefer ns-3-dev). 

## Install ns3 addon files

Now suppose you have successfuly installed ns-3-dev, you can start to install RL4Net.  

1. Install zmqbridge and protobuf
   ns3-env needs ZMQ and libprotoc, you can install as follow:  

   ```bash
   # to install protobuf-3.6 on ubuntu 16.04:
   sudo add-apt-repository ppa:maarten-fonville/protobuf
   sudo apt-get update
   apt-get install libzmq5 libzmq5-dev
   apt-get install libprotobuf-dev
   apt-get install protobuf-compiler
   ```

2. Install addon files
   Since you have installed dependence libs, you can install addon files by:  

    ```bash
    python ns3_setup.py --wafdir=YOUR_WAFPATH
    ```

    the `YOUR_WAFPATH` is correspond to the introduction of ns-3 installation, where you can execute `./waf build`, typically `ns-3-allinone/ns-3-dev`. Remember to use absolute path.  

    The default value of  wafdir is `/ns-3-dev` (notice it is subdir of '/'). As an alternative, you can copy the folder into `/ns-3-dev`, then run  

    ```bash
    python ns3_setup.py
    ```

## Install pyns3
pyns3 is the python module that connect python and ns3. Use pip(or pip3) to install this module with your python env(maybe conda).  

```bash
pip install ns3-env/ns3-python-connector
```

## Install wjwgym
wjwgym is a lab that helps build reinforcement learning algorithms. See: [Github](https://github.com/LampV/Reinforcement-Learning)  
Install it with pip and your python env:  

```bash
pip install RL4Net-lib/wjwgym-home
```

The lab need numpy, torch and tensorboard. You can pre-install them, especially pytorch, by which you can choose pip/conda.

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
