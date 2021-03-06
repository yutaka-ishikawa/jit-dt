JIT-DT (Just-In-Time Data Transfer)
					      May 12, 2019
					      System Software Research Team
					      System Software Development Team
					       			   RIKEN R-CCS

1) Requirement libraries
  - libmicrohttdpd for server
  - libcurl for client
  $ sudo apt-get install curl libc6 libcurl3 zlib1g

2) Installation
  2-1) For client only
   $ make client
  2-2) For server only
   $ make server
  2-3) For both client and server
   $ make

3) How to use watch_and_transfer
  The watch_and_transfer tool provides http, scp, and sftp protocols.
  3-1) An example of http
   $ ./watch_and_transfer http://xxx.r-ccs.riken.jp <watching directory path> -v

  3-2) An example of scp
   In case of keeping the same file name in the remote machine
   $ ./watch_and_transfer scp:XXXX@YYY.ZZZ.jp ~/tmp/ -v

   In case of the remote file name is specified. In this case, the remote
   file is overwritten when a new data is transferred.
   $ ./watch_and_transfer scp:XXXX@YY.ZZZ.jp:data ~/tmp/ -v

  3-3) An example of sftp
   In case of keeping the same file name in the remote machine
   $ ./watch_and_transfer sftp:XXXX@YY.ZZZ.jp ~/tmp/ -v

   In case of the remote file name is specified. In this case, the remote
   file is overwritten when a new data is transferred.
   $ ./watch_and_transfer sftp:XXXX@YY.ZZZ.jp:data ~/tmp/ -v

  3-4) The -s option is used in case of the following usage:
     A file is created under the directory whose path represents created time.
     For example, a file created at 2016/08/14/15:10 is created
     under the /mnt/data/2016-0814/15/10, and another file created
     at 2016/08/14/16:00 is located under the /mnt/data/2016-0814/16/00.
     If the following command starts the data transfer at 16:00 on 2016/08/14.
     $ ./watch_and_transfer sftp:XXXX@YY.ZZZ:data \
       /mnt/data/ -s 2016-0814/20/ -v
     That is, it watches directories: /mnt/data/, /mnt/data/2016-08-14,
     and /mnt/data2016-0814/20/.


If you have questions, send an email to syssoft@ml.riken.jp

