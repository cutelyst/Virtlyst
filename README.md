# Virtlyst
Web interface to manage virtual machines with libvirt

Don't let your virtualization management use more resources than your main virtualized needs.

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

Default Username: admin
Password: admin
