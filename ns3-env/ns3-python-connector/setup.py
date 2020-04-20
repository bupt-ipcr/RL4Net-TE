from setuptools import setup, find_packages
import sys
import os.path
import os
import re
cwd = os.getcwd()


def readme():
    with open('README.md') as f:
        return f.read()


setup(
    name='pyns3',
    version='0.2.0',
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
