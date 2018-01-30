# Dev Setup for Samseg Python Port

These instructions apply to a freshly built and upgraded machine running Ubuntu 16.04 and is intended as a reference guide. For a Linux system already set up for develpment, many of these steps can be skipped or appropriately altered.
## Create a dev environment:

```
sudo apt-get install build-essential \
            tcsh \
            libtool-bin \
            libtool \
            automake \
            gfortran \
            libglu1-mesa-dev \
            libfreetype6-dev \
            uuid-dev \
            libxmu-dev \
            libxmu-headers \
            libxi-dev \
            libx11-dev \
            libxml2-utils \
            libxt-dev \
            libjpeg62-dev \
            libxaw7-dev \
            liblapack-dev
sudo apt-get install tcsh
sudo apt-get install gcc-4.8 g++-4.8 libgfortran-4.8-dev
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 50
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 50
sudo apt-get install python3-dev
```
Most recent _ITK_ requires cmake 3.95 or greater. Test with 

`cmake --version`.

Download and install `cmake-3.10.2-Linux-x86_64.sh`
You may need to copy it to standard location to get it working.
```
sudo cp -r bin /usr/
sudo cp -r doc /usr/share/
sudo cp -r man /usr/share/
sudo cp -r share /usr/
```
## Acquire Source Code
Create a working directory. This document this will refer to it as `~/work/cm`.
```angular2html
cd ~/work/cm
```
Get the latest ITK:
```angular2html
git clone https://itk.org/ITK.git
```
Get the porting branch of FreeSurfer and install pybind11:
```angular2html
git clone git@github.com:innolitics/freesurfer.git
cd freesurfer
git checkout  nf-gems2-python-port
cd GEMS2
git clone https://github.com/pybind/pybind11.git
```
Also required is the prebuilt packages:
```angular2html
wget ftp://surfer.nmr.mgh.harvard.edu/pub/dist/fs_supportlibs/prebuilt/centos6_x86_64/centos6-x86_64-packages.tar.gz
tar -xzvf centos6-x86_64-packages.tar.gz
```
## Build from Source
### Build ITK
```angular2html
cd ~/work/cm
mkdir ITK-build
cd ITK-build
cmake ../ITK
make -j4
```
### Build FreeSurfer
```
export ITK_DIR=~/work/cm/ITK-build
cd ~/work/cm/freesurfer
./setup_configure
./configure --disable-Werror --with-pkgs-dir=~/work/cm/centos6-x86_64-packages --disable-xawplus-apps --disable-tcltk-apps
make -j4
```
### Build GEMS2python
In the GEMS2 directory run the ccmake utility
```cd ~/work/cm/freesurfer/GEMS2
ccmake .
```
Set the `CMAKE_CXX_FLAGS` and `CMAKE_C_FLAGS` to "`-fPIC -fpermissive`".

`BUILD_EXECUTABLES` `BUILD_GUI` `BUILD_MATLAB` `BUILD_SHARED_LIBS` and `BUILD_TESTING` should be `OFF`.

Check that the `PYTHON_EXECUTABLE` and `PYTHON_LIBRARY` have valid values such as `/usr/bin/python3.5` 
and `/usr/lib/x86_64-linux-gnu/libpython3.5m.so` respectively.

Build the GEMS2 code with:
```angular2html
cmake .
make -j4
```
## Python Setup
Install the python virtual environment tools:
```angular2html
sudo apt-get install python-pip
sudo apt-get install python3-pip
pip install --upgrade pip
pip install --user virtualenv
pip install --user virtualenvwrapper
```
To take full advantage add this to the end of your `.bashrc` file in your home directory:
```angular2html
export ITK_DIR=$HOME/work/cm/ITK-build
# where to store our virtual envs
export WORKON_HOME=$HOME/virtenvs
# where projects will reside
export PROJECT_HOME=$HOME/Projects-Active
# where is the virtualenvwrapper.sh
source $HOME/.local/bin/virtualenvwrapper.sh
```
Locate the python 3 interpreter executable:
```angular2html
which python3
```
For purposes of this document ```/usr/bin/python3``` will the presumed location.

Now create a `gems2` virtual environment using the correct location of your python3:
```angular2html
mkvirtualenv gems2 -p /usr/bin/python3
```
The command prompt will indicate you are using this environment with a `(gems2)` prefix.
Having created `gems2` you can enter this environment in any terminal shell by using:
```angular2html
workon gems2
```
At this point any python packages installed will be local to that environment 
and not interfere with any other python projects, which is the purpose of using virtual environments.

Verify with:
```
which python
```
You should see something like `~/virtenvs/gems2/bin/python3`

Install the python requirements:
```angular2html
pip install -r as_python/requirements.txt
```
## Test and Run
## Running Samseg Code
...
## Running Test Scripts
...
