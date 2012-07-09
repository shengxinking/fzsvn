/*
 *  file: libinf_ioctl.h
 *  it defined API for manipulate NIC interface using ioctl command
 *
 *  write by Forrest.zhang
 */

#ifndef __NETDEVICE_H__
#define __NETDEVICE_H__

#include <netinet/in.h>
#include <linux/if.h>

#ifdef __cplusplus
extern "C" {
#endif /* end of __cplusplus */

    extern const char *if_error(void);

    extern int if_get_index(const char *ifname);

    extern const char *if_get_name(int index, char *ifname, size_t size);
    extern int if_set_name(const char *name, const char *newname);

    extern struct sockaddr *if_get_hwaddr(const char *ifname, struct sockaddr *addr);
    extern int if_set_hwaddr(const char *ifname, const struct sockaddr *addr);

    extern int if_get_mtu(const char *ifname);
    extern int if_set_mtu(const char *ifname, int mtu);

    extern struct sockaddr *if_get_addr(const char *ifname, struct sockaddr *addr);
    extern int if_set_addr(const char *ifname, const struct sockaddr *addr);

    extern struct sockaddr *if_get_netmask(const char *ifname, struct sockaddr *addr);
    extern int if_set_netmask(const char *ifname, const struct sockaddr *addr);

    extern struct sockaddr *if_get_brdaddr(const char *ifname, struct sockaddr *addr);
    extern int if_set_brdaddr(const char *ifname, const struct sockaddr *addr);

    extern struct sockaddr *if_get_dstaddr(const char *ifname, struct sockaddr *addr);
    extern int if_set_dstaddr(const char *ifname, const struct sockaddr *addr);

    extern int if_get_alladdr(const char *ifname, struct ifconf *ifc);

    extern int if_is_up(const char *ifname);
    extern int if_up(const char *ifname);
    extern int if_down(const char *ifname);  

    /* detect network cable is plugged */
    extern int if_is_link_up(const char *ifname);

    extern int if_is_noarp(const char *ifname);
    extern int if_noarp(const char *ifname);
    extern int if_arp(const char *ifname);

    extern int if_is_debug(const char *ifname);
    extern int if_debug(const char *ifname);
    extern int if_nodebug(const char *ifname);

    extern int if_is_promisc(const char *ifname);
    extern int if_promisc(const char *ifname);
    extern int if_nopromisc(const char *ifname);

    extern int if_is_dynamic(const char *ifname);
    extern int if_dynamic(const char *ifname);
    extern int if_nodynamic(const char *ifname);
    
    extern int if_is_broadcast(const char *ifname);
    extern int if_is_loopback(const char *ifname);
    extern int if_is_ppp(const char *ifname);
    extern int if_is_running(const char *ifname);
    extern int if_is_notrailers(const char *ifname);
    extern int if_is_allmulti(const char *ifname);
    extern int if_is_master(const char *ifname);
    extern int if_is_slave(const char *ifname);
    extern int if_is_multicast(const char *ifname);
    extern int if_is_portsel(const char *ifname);
    extern int if_is_automedia(const char *ifname);


#ifdef __cplusplus
}
#endif /* end of __cplusplus */

#endif /* end of __LIBINF_IOCTL_H__ */
