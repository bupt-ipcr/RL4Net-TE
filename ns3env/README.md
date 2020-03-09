# ns3-gym-brief

[ns3-gym](https://github.com/tkn-tub/ns3-gym) 是基于ns-3和openai-gym开发的一个框架，作者利用Gym的特性，编写了一个名为 *ns3-v0* 的Gym环境，其实际用途为同ns-3交互。通过编写ns-3脚本就可以改变环境，通过更改gym代码则可以使用不同的强化学习算法。

但是ns3-gym代码与ns3代码并没有接耦，一方面造成了对ns3版本的实质依赖，一方面有大量重复代码。为此我将ns3-gym代码中有效代码抽出来做成brief包，添加了用于辅助安装的脚本，并为避免必须在指定文件夹下启动代码做了少数调整。

Installation
============

1. 正确安装 ns-3及依赖、ns3-gym依赖项

   ns-3的安装方法参考 [ns-3](https://www.nsnam.org/wiki/Installation#Installation) 官方引导

   ns3-gym对ZMQ和protoc有依赖，可以参照以下方式安装

   ```bash
   # to install protobuf-3.6 on ubuntu 16.04:
   sudo add-apt-repository ppa:maarten-fonville/protobuf
   sudo apt-get update
   
   apt-get install libzmq5 libzmq5-dev
   apt-get install libprotobuf-dev
   apt-get install protobuf-compiler
   ```

   

   **注意：**通过这个方式下载的protobuf版本是3.6.1，如果需要更高等级的版本请参考 [google/prorocolbuffers](google/prorocolbuffers) 的引导

   

2. 从git获取ns3-gym-brief的源代码

   ```bash
   git clone https://github.com/LampV/ns3gym-brief ns3gym_brief
   cd ns3gym_brief
   ```

   

3. 使用安装脚本将opengym文件夹复制到ns3路径下，重新配置编译

   ```bash
   python setup.py --wafdir=YOUR_DIR
   ```

   `--wafdir`的默认值是`/ns-3-dev`

   你可以通过

   ```bash
   python setup.py --help
   ```

   查看setup.py的完整参数说明

   **注意：**执行该指令的时候请确保你已经安装了protoc，并且尽量不要在conda环境下，否则可能出现protoc版本混乱等问题。

   

4. 安装ns3gym

   ```bash
   pip install ns3gym
   ```

   **注意：**这一步应该按照你的python环境选择 pip/pip3；如果你使用conda，应该先activate相应环境



---



> How to reference ns3-gym?
>
> Please use the following bibtex :
>
> ```
> @inproceedings{ns3gym,
>   Title = {{ns-3 meets OpenAI Gym: The Playground for Machine Learning in Networking Research}},
>   Author = {Gaw{\l}owicz, Piotr and Zubow, Anatolij},
>   Booktitle = {{ACM International Conference on Modeling, Analysis and Simulation of Wireless and Mobile Systems (MSWiM)}},
>   Year = {2019},
>   Location = {Miami Beach, USA},
>   Month = {November},
>   Url = {http://www.tkn.tu-berlin.de/fileadmin/fg112/Papers/2019/gawlowicz19_mswim.pdf}
> }
> ```



