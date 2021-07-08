SET lappy=10.10.12.33

cd C:\Users\mur1_\source\repos\clusterfq\clusterfq\
"C:\Program Files\PuTTY\pscp.exe" -i "C:\Users\mur1_\Desktop\w10.ppk" -r C:\Users\mur1_\source\repos\clusterfq\clusterfq\ mur1@%lappy%:/home/mur1/repos/clusterfq/
"C:\Program Files\PuTTY\putty.exe" -ssh -2 -i "C:\Users\mur1_\Desktop\w10.ppk" -m c:remote_make.cmd mur1@%lappy%
"C:\Program Files\PuTTY\pscp.exe" -i "C:\Users\mur1_\Desktop\w10.ppk" -r C:\Users\mur1_\source\repos\clusterfq\x64\Release\clusterfq.html mur1@%lappy%:/home/mur1/repos/clusterfq/
"C:\Program Files\PuTTY\pscp.exe" -i "C:\Users\mur1_\Desktop\w10.ppk" -r C:\Users\mur1_\source\repos\clusterfq\x64\Release\favicon.ico mur1@%lappy%:/home/mur1/repos/clusterfq/
"C:\Program Files\PuTTY\pscp.exe" -i "C:\Users\mur1_\Desktop\w10.ppk" -r C:\Users\mur1_\source\repos\clusterfq\x64\Release\js mur1@%lappy%:/home/mur1/repos/clusterfq/
"C:\Program Files\PuTTY\pscp.exe" -i "C:\Users\mur1_\Desktop\w10.ppk" -r C:\Users\mur1_\source\repos\clusterfq\x64\Release\css mur1@%lappy%:/home/mur1/repos/clusterfq/
"C:\Program Files\PuTTY\pscp.exe" -i "C:\Users\mur1_\Desktop\w10.ppk" -r C:\Users\mur1_\source\repos\clusterfq\x64\Release\img mur1@%lappy%:/home/mur1/repos/clusterfq/

cd C:\Users\mur1_\source\repos\clusterfq\clusterfq_cl\
"C:\Program Files\PuTTY\pscp.exe" -i "C:\Users\mur1_\Desktop\w10.ppk" -r C:\Users\mur1_\source\repos\clusterfq\clusterfq_cl\ mur1@%lappy%:/home/mur1/repos/clusterfq_cl/
"C:\Program Files\PuTTY\putty.exe" -ssh -2 -i "C:\Users\mur1_\Desktop\w10.ppk" -m c:remote_make.cmd mur1@%lappy%

cd C:\Users\mur1_\source\repos\clusterfq\clusterfq_shell\
"C:\Program Files\PuTTY\pscp.exe" -i "C:\Users\mur1_\Desktop\w10.ppk" -r C:\Users\mur1_\source\repos\clusterfq\clusterfq_shell\ mur1@%lappy%:/home/mur1/repos/clusterfq_shell/
"C:\Program Files\PuTTY\putty.exe" -ssh -2 -i "C:\Users\mur1_\Desktop\w10.ppk" -m c:remote_make.cmd mur1@%lappy%