# Virtlyst
Web interface to manage virtual machines with libvirt

Don't let your virtualization management use more resources than you main virtualized needs.

 * Ideal for small deployments where simplicity is key to virtualization usage
 * Low memory footprint, around 5 MiB of RAM used
 * Low CPU usage
 * Look and feel easly customized with templates
 
# Screenshot

![Instances](http://i67.tinypic.com/161yn1d.png)

# Running

    cutelyst-wsgi2 \
    --application path_to_libVirtlyst.so \
    --chdir2 _path_to_Virtlyst_clone \
    --static-map /static=root/static \
    --http-socket localhost:3000 \
    --master

# Running with Docker on Linux Mint 19/Ubunt 18.04
##ToDo
* Add support for console
* Streamline process

## Process
* Make sure Docker is installed  
* Clone the repo: git clone https://github.com/cutelyst/Virtlyst.git virtlyst
* Build the image and name it: cd virtlyst; docker -t virtlyst build .
* Once the build is done you'll have a image called virtlyst
* To be able to access the hosts libvirt and the web panel some stuff has to be mapped
  docker run -ti --name virtlyst -p 3000:3000 -v /run/libvirt:/run/libvirt:rw virtlyst
