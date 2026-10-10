#!/bin/bash
 
function file_del_comment()
{
    #delete the line begin with '#'
    sed -i "/^[ \t]*\#/d" $file  

    #delete the line end with '#'
   sed -i "s/\#[^\"]*//" $file

}
 
function dir_del_comment()
{
    for file in `ls `; do   
        case $file in      
        *.txt)                
            file_del_comment
            ;;
        *.cfg)    
            file_del_comment
            ;;
        *)                      
            if [ -d $file ]; then 
                cd $file 
                dir_del_comment
                cd ..
            fi
        ;;
    esac
    done
}
 
DIR_FILE=$1
 
if [ -d $DIR_FILE ]; then    
    cd $DIR_FILE               
    dir_del_comment  
    exit 0;
fi

if [ -f $DIR_FILE ]; then
    file=`basename $DIR_FILE`   
    if [[ `echo $DIR_FILE | grep /` == $DIR_FILE ]]; then
        cd `echo $DIR_FILE | sed -e "s/$file//"` 
        file_del_comment
    else
        file_del_comment
    fi
    exit 0;
fi
