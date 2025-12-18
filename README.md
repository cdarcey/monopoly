# Pilot Light Template


## Information
This repository acts as a template for creating a [Pilot Light](https://github.com/PilotLightTech/pilotlight) application using the engine source. In the future, a
binary template will be created.

## Build

As a first step, clone and build **Pilot Light**. Here we are assuming it is adjacent to this repo.

### Windows

From within a local directory, enter the following commands in your terminal:
```bash
# clone & build pilot light
git clone https://github.com/PilotLightTech/pilotlight
cd pilotlight/src
build_win32.bat

# clone & build example
cd ..
git clone https://git.pilotlight.tech/pilotlight/pl-template
cd pl-template/scripts
python setup.py
python gen_build.py
cd ../src
build.bat
```

### Linux

From within a local directory, enter the following commands in your terminal:
```bash
# clone & build pilot light
git clone https://github.com/PilotLightTech/pilotlight
cd pilotlight/src
chmod +x build_linux.sh
./build_linux.sh

# clone & build example
cd ..
git clone https://git.pilotlight.tech/pilotlight/pl-template
cd pl-template/scripts
python3 setup.py
python3 gen_build.py
cd ../src
chmod +x build.sh
./build.sh
```

### MacOS

From within a local directory, enter the following commands in your terminal:
```bash
# clone & build pilot light
git clone https://github.com/PilotLightTech/pilotlight
cd pilotlight/src
chmod +x build_linux.sh
./build_linux.sh

# clone & build example
cd ..
git clone https://git.pilotlight.tech/pilotlight/pl-template
cd pl-template/scripts
python3 setup.py
python3 gen_build.py
cd ../src
chmod +x build.sh
./build.sh
```

Binaries will be in _pilotlight/out/_.

Run the application by pressing F5 if using VSCode or manually like so:
```bash
# windows
pilot_light.exe -a template_app

# linux/macos
./pilot_light -a template_app
```