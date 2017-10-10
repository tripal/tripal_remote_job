# tripal-remote-job

# Currently Working:
  - [x] SSH
  - [x] PBS
  - [x] SLURM
  - [x] GridEngine
  
  Email Notification Status: Not supported
  
# Requirements
Tripal 3 (Tested with Tripal 3 [7.x-3.0-rc1]) 

1. Install module as any other module
2. From Admin menu, visit 'Tripal > Extensions > Remote Computational Resources' to set up one or more remote servers
3. Have your module call tripal_add_remote_job() with following arguments:
   -Argument:    Job Name
    Type:        String
    Required:    Yes
   -Argument:    Command
    Type:        String
    Required:    Yes
   -Argument:    Files
    Type:        Array
    Required:    Optional (Required if specifying Email Notification)
   -Argument:    Email Notification
    Type:        Boolean
    Required:    Optional
       
 Examples:
    
 tripal_add_remote_job("myCoolRemoteJob", "uptime >> coolLevel.txt", array("/local/Path/to/coolLevel.txt"), FALSE);
 ^   Executes regular system command 'uptime' and appends its output to coolLevel.txt without notification via Email
    
 tripal_add_remote_job("simpleUptime", "uptime");
 ^   Executes regular system command 'uptime'. 
     Its output will be returned within STDOUT.txt. No additional files were used and no email notification was sent.
       
