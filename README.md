# ftree
A kind of combination of 'find' and 'tree' commands for Linux distributions

### after running ftree.c:

```
path{Enter}
```
_tree all files/folders recursively under that path_
```
path{Space}-dir{Space}SearchedDir
```
_tree and give the path of Searched Folders_
```
path{Space}-file{Space}SearchedFile
```
_tree and give the path of Searched Folders_

>A part of file/folder name or only file extension is enough for searching

>It is case sensitive

>Be careful of spaces.\
>path{Space}-file{Space}{Space} &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; will search files which contain one {Space} in their names.

### Examples:

![](https://user-images.githubusercontent.com/114924089/194725982-0c3e35a7-1315-4ee9-9912-862a5e316654.png)

![](https://user-images.githubusercontent.com/114924089/194725985-a0c43670-15cd-4e4d-ac95-384fefed655a.png)

![](https://user-images.githubusercontent.com/114924089/194725990-cb7decdb-fd24-48a2-8e54-1a7989351af2.png)

> You may modify and source .bashrc file to run the code quickly

```
alias ftree='cd {path to output file} && ./ftree'
```


