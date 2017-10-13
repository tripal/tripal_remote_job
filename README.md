# tripal-remote-job
Allows Tripal to run jobs on other machines or clusters
Ported from GenSAS to work with Tripal

## Status
  - [x] SSH
  - [ ] PBS
  - [ ] SLURM
  - [ ] GridEngine
  - [ ] Email Notification
  - [ ] Allow user to specify SSH Key locations
  - [ ] Toggle ability of user to prevent remote files from being deleted (Default: disabled, files will always be deleted)
  - [ ] API argument to specify target destination for job if multiple (make some logic to dynamically asign?)
  
## Requirements
Tripal 3 (Tested with Tripal 3 [7.x-3.0-rc1]) 
## Installation
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
       
