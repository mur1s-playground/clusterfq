cd C:\Users\mur1_\source\repos\clusterfq\clusterfq\
"C:\Program Files\PuTTY\pscp.exe" -i "C:\Users\mur1_\Desktop\w10.ppk" -r C:\Users\mur1_\source\repos\clusterfq\clusterfq\ mur1@192.168.246.5:/home/mur1/repos/clusterfq/
"C:\Program Files\PuTTY\putty.exe" -ssh -2 -i "C:\Users\mur1_\Desktop\w10.ppk" -m c:remote_make.cmd mur1@192.168.246.5
"C:\Program Files\PuTTY\pscp.exe" -i "C:\Users\mur1_\Desktop\w10.ppk" -r C:\Users\mur1_\source\repos\clusterfq\x64\Release\clusterfq.html mur1@192.168.246.5:/home/mur1/repos/clusterfq/
"C:\Program Files\PuTTY\pscp.exe" -i "C:\Users\mur1_\Desktop\w10.ppk" -r C:\Users\mur1_\source\repos\clusterfq\x64\Release\js mur1@192.168.246.5:/home/mur1/repos/clusterfq/