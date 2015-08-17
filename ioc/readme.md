# Quick start
__NOTE__ that any modifications you do for your local testing must never be installed in the global $TEMPLATES folder using the installation script!

## Using default (global) BSREAD library

1. Open `ioc/example.startup` and change the `SYS` variable, eg.:

        epicsEnvSet SYS MYTEST
        
2. run `iocsh example.startup` in the `ioc` folder:

        cd ioc
        iocsh example.startup

## Using local BSREAD library

1. Open `ioc/example.startup` and change the `SYS` variable, eg.:

        epicsEnvSet SYS MYTEST
        
2. Build local library (using driver.makefile), by running `make` in the top folder. By default, the library will build using you username as version description, eg.: `libBSREAD-username.so`

3. In order for the example IOC to load your library, you need to:
    - make a symbolic link to the built library. The following example uses the library build for `SL6-x86_64` architecture:
            
            cd ioc
            ln -s ../O.3.14.12_SL6-x86_64/ bin
            
    - tell the example IOC to use newly built library version, by changing the `require` command in `ioc/templates/bsread.startup` to:
    
        require "BSREAD","username"
            
2. run `iocsh example.startup` in the `ioc` folder:

        iocsh example.startup