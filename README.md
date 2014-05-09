FTP_project
===========

FTP client Server implementation on Amazon AWS.
************************************************************************************
Introduction:
************************************************************************************
 A simple FTP project for file transfer from client running on any Linux and supporting FTP commands like: pwd, cd , lmdir,, and performs under mltithreading paradigm where each client connected is a separated thread.
Each server and client side code takes care of error handling.
The valid users that are to be entered to gain FTP access are:
Alice:000
Bob:111
****************************************************************************
HOW TO RUN
***************************************************************************
assuming you are running this under linux distro as a client and a linux AMI running on AWS:
try to SSH from local machine into your linux ubuntu instance running on EC2 as:

$ sudo su
# ssh -i instancepemfile.pem ubuntu@PUBLIC_IP_OF_Instance
for instance say your public IP of instance that is running (make sure instance is running and has 2/2 checks showing on status bar..) is 54.187.87.222 
now on linux term, cd to location where instance .pem file is located lets say you download pem file in Downloads folder of Ubuntu...
.pem is SSH keyfile given the time you created your instance, you may download later...
Note: .pem file is the only thing thats giving you access to your instance ..please save and make copies in different locations you use as backup..you are gonna need it a lot..
now back to ssh, so say you want to use ubuntu running on aws , ok...there is only one way...SSH...because the installed instance only supports terminal functions..there is no GUI or Desktop view to that like you normally have for ubuntu on local machines.
.so ssh to this IP :
/home/imran/Downloads$ sudo su
# ssh -i instance1.pem ubuntu@54.187.87.222 
please check your public ip assigned to your EC2 instance.
here i arbitrarily took 54.187.87.222
now after you ssh ,
scp (scp the file your instance ) 
this will be explained in following section
******************************************************************
complining and run
***********************************************************************
ok first things first:
copy the server IP address in
