# SMB/CIFS_discovery

Basic Description
---

Embedded utility to discover SMB/CIFS servers in local area network. Currently work on MIPS lower ended.

Detailed Description
---

This repository is to implemented a simple utility with SMB/CIFS discover and top directory listing. This utility is designed for embedded system, whose ROM resources are quite limited.

This utility use JSON string for outputting, which is very easy to redirect to pipe, UNIX domain socket, files and other inter-process-commucations.

Problem and Solution
---

I had met a mysterious problem. Here is the [detailed question on StackOverflow](http://stackoverflow.com/questions/41034511/c-function-parameter-mysteriously-drifted#)

I have solved this problem by overriding Samba smbclient (samba-3.6.25/source3/client) sources with my own ones. This operation makes my own project be built at the same environment with Samba sources itself. And finally, the mysterious problem disapeared. 

However, this did not solve the fundamental problem. So any person who know why my original problem occurred, please feel welcomed to comment on the StackOverflow site. Or [send me an Email](mailto:laplacezhang@126.com).

Thanks at advance!



