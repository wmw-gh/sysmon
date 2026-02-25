#pragma once

extern void net_init(void);
extern void	net_read_ip(WCHAR* ip);
extern void	net_read_gateway(WCHAR* gateway);
extern void	net_read_dhcp(WCHAR* dhcp);
extern void net_read_ping(ULONG* ping_ms);
