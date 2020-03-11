<!--
 * @author       : Jiawei Wu
 * @create time: 2020-01-15 16:42
 * @edit time    : 2020-01-15 16:57
 * @file: /README.md
 -->
# 强化学习工具包  

基于`Pytorch`与`OpenAI Gym`实现强化学习的工具包  

## 安装  

注意： 工具包有使用pytorch和numpy，建议使用conda新建环境后安装。  

1. 安装工具包  
   从GitHub下载包  

   ```bash
   git clone https://github.com/LampV/Reinforcement-Learning
   ```

   进入文件夹

   ```bash
   cd Reinforcement-Learning
   ```

   安装wjwgym到本地

   ```bash
   pip install wjwgym-home
    ```

2. 运行示例  

   ```bash
   python examples.dqn.py
   ```

   若程序正常运行，说明安装成功

## 使用  

1. agents  
   通过`wjwgym.agents`中提供的基类可以创建自己的强化学习智能体，其通用方法如下：  

   ```python
   # import 基类
   from wjwgym.agents import xxxBase
   # 继承基类并实现必要的函数  
   class myxxx(xxxBase):  
       def _build_net(self):
           pass
   ```

   具体的使用方式在`examples/`下都能找到代码示例和注释文档

2. models  
   要调用简单的pytorch神经网络结构作为DRL的神经网络，只需要`import`即可  

   ```python
   from wjwgym.models import SimpleDQNNet
   ```

3. envs  
   要调用附带的envs，需要让 `__init__.py` 中的代码执行以注册到 `gym`，之后按照标准的`gym`方式创建即可：  

   ```python
   import wjwgym  
   env = gym.make('Maze-v0)
   ```
