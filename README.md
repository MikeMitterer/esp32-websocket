# ESP32 WebSocket

## Removing sensitive data from a repository
> https://help.github.com/articles/removing-sensitive-data-from-a-repository/

    # Step 1
    bfg --delete-files YOUR-FILE-WITH-SENSITIVE-DATA
    
    # Step 2
    git reflog expire --expire=now --all && git gc --prune=now --aggressive
    
    # Step 3
    git pull
    
    # Step 4
    git push -f origin master
    