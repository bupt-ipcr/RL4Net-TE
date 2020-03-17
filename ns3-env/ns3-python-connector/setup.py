from setuptools import setup, find_packages
import sys
import os.path
import os
import re
import pyns3
cwd = os.getcwd()
protobufFile = cwd + '/pyns3/messages_pb2.py'

if not os.path.isfile(protobufFile):
    # 如果没有protocbuf文件，则尝试产生
    rc = os.system('whereis protoc')
    # 先检查是否有whereis指令
    if rc != 0:
        print("Command: ", "whereis protoc", "not found.")
        sys.exit("Missing command 'whereis', please check!")
    protoc = None
    # 再查找protoc指令
    rt = os.popen("whereis protoc").read()
    protoc_paths = re.finditer(r'/[^\s]+', rt)
    for protoc_path in protoc_paths:
        if 'conda' in protoc_path[0]:
            continue
        else:
            protoc = protoc_path[0]
            break
    # 如果没有找到protoc，则需要报错
    if protoc is None:
        print("File: ", "messages_pb2.py", " was not found.")
        sys.exit('Protocol Buffer messages are missing. Please run ./waf configure to generate the file')
    # 如果有，则尝试产生文件
    print('generate proto file')
    os.system(f'{protoc} --python_out=./pyns3 -I=./pyns3 messages.proto')

def readme():
    with open('README.md') as f:
        return f.read()


setup(
    name='pyns3',
    version='0.1.0',
    packages=find_packages(),
    scripts=[],
    url='',
    license='MIT',
    author='Piotr Gawlowicz',
    author_email='gawlowicz.p@gmail.com',
    description='Python meets ns-3',
    long_description='Python meets ns-3',
    keywords='Python, ML, RL, ns-3',
    install_requires=['pyzmq', 'numpy', 'protobuf', 'gym'],
    extras_require={},
)
