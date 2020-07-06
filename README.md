# mini_xdr_rpc
Mini ONC RPC for compiling DOOCS stuff  
We will use RepoRoot word for repository root directory.  
  
## projects  
 1.  initial_doocs_win_xdr_rpc.vcxproj => dynamic library with xdr_* functions  
 2.  final_doocs_win_xdr_rpc.vcxproj   => static library that includes dynamic 'initial_doocs_win_xdr_rpc' + adds functions for auth and client_*


## Compile library  
Open developer command prompt and run there command  
```bat  
build_lib.bat  
```  
  
## artifacts location  
'$(RepoRoot)\sys\$(TargetPlatform)\lib' and '$(RepoRoot)\sys\$(TargetPlatform)\dll'  
### target platforms  
 1.  ARM  
 2.  ARM64  
 3.  x64  
 4.  x86  
 
 
 ## notes  
 Probably later on when we will switch to compilation of DOOCS server library we will need to have static library, because the code for auth depends on getuid, getid functions, that will be implemented later.  
On windows this possible to do (at least to my knowledge) only with static library.  
 Currently for DOOCS client library we do not use any XDR RPC function that requires getuid, getgid familiy functions.
 
