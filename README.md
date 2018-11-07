# tripal_remote_job
Allows Tripal to run jobs on other machines or clusters

Ported from GenSAS to work with Tripal

## Status
  - [ ] SSH
    - [x] Initiate Job
    - [ ] Get status of job
    - [x] Get results of job
  - [ ] PBS
  - [ ] SLURM
  - [ ] GridEngine
  - [x] Email Notification
    - [x] Job initiated (generic)
    - [x] Job complete (generic)
    - [x] Custom email specified by origin module
  - [ ] Allow user to specify SSH Key locations
  - [ ] Toggle ability of user to prevent remote files from being deleted (Default: disabled, files will always be deleted)
  - [ ] API argument to specify target destination for job if multiple (make some logic to dynamically asign?)
  
## Requirements
Tripal 3 (Tested with Tripal 3 [7.x-3.0-rc1])
libraries module
drushd module

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
   -Argument:    Origin Module - Used so that the Mail functionality
    Type:        String
    Required:    Yes
   -Argument:    Files
    Type:        Array
    Required:    Optional (Required if specifying Email Notification)
   -Argument:    Email Notification
    Type:        Boolean
    Required:    Optional
       
 Examples:
    
 tripal_add_remote_job("myCoolRemoteJob", "uptime >> coolLevel.txt","my_module" array("/local/Path/to/coolLevel.txt"), FALSE);
    Executes regular system command 'uptime' and appends its output to coolLevel.txt without notification via Email
    
 tripal_add_remote_job("simpleUptime", "uptime", "my_module");
    Executes regular system command 'uptime'. 
    Its output will be returned within STDOUT.txt. No additional files were used and no email notification was sent.
     
## Remote Resource Requirements
Refer to this section when configuring your remote resources
#### SSH / Stand-alone Computational resource
  -Any commands to be called must either exist on the $PATH of the configured remote user or be passed along with the contents of the job

## Mail Functionality
When creating a job, Tripal Remote Job is aware of what module (origin module) has spawned the job (via the API call).
It will then query that module to see if there are any specific mail functionality required for notifying the user in two conditions:
    -Job is launched
    -Job has completed
These require that the module's hook_mail function has provisions for the following keys:
    `job-started`
    `job-completed`
If these keys exist, the module's hook_mail function will be used. Else, Tripal Remote Job will send a generic email about job status. This functionality gives the origin module the ability to specify a template for the emails to be sent when a job starts or finishes.
**Modules that use Tripal Remote job are not required to provide custom mail functionality**