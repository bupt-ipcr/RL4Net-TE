# ns3-env

为了使python能直接将ns3视为简单环境，编写了与ns3进行通信和抽象的代码

Installation
============

1. 正确安装 ns-3及依赖项

   ns-3的安装方法参考 [ns-3](https://www.nsnam.org/wiki/Installation#Installation) 官方引导

   ns3-env对ZMQ和protoc有依赖，可以参照以下方式安装

   ```bash
   # to install protobuf-3.6 on ubuntu 16.04:
   sudo add-apt-repository ppa:maarten-fonville/protobuf
   sudo apt-get update
   
   apt-get install libzmq5 libzmq5-dev
   apt-get install libprotobuf-dev
   apt-get install protobuf-compiler
   ```

   

   **注意：**通过这个方式下载的protobuf版本是3.6.1，如果需要更高等级的版本请参考 [google/prorocolbuffers](google/prorocolbuffers) 的引导

   

2. 进入ns3-env文件夹下

   ```bash
   cd ns3-env/
   ```

   

3. 使用安装脚本将env-interface文件夹复制到ns3路径下，重新配置编译

   ```bash
   python env_setup.py --wafdir=YOUR_DIR
   ```

   `--wafdir`的默认值是`/ns-3-dev`

   你可以通过

   ```bash
   python env_setup.py --help
   ```

   查看env_setup.py的完整参数说明

   **注意：**执行该指令的时候请确保你已经安装了protoc，并且尽量不要在conda环境下，否则可能出现protoc版本混乱等问题。


4. 安装python库：pyns3

   ```bash
   pip install ./ns3-python-connector
   ```

   **注意：**这一步应该按照你的python环境选择 pip/pip3；如果你使用conda，应该先activate相应环境
