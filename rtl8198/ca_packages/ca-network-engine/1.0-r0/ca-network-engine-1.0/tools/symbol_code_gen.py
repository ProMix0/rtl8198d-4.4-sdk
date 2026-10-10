import ply.lex as lex
import os
import sys
import re
import copy


###############################################################################
# C header file paraser
###############################################################################

tokens = [
    'NUMBER',
    'NAME',
    'OPEN_PAREN',
    'CLOSE_PAREN',
    'OPEN_BRACE',
    'CLOSE_BRACE',
    'SEMI_COLON',
    'COMMA',
    'COMMENT_SINGLELINE',
    'COMMENT_MULTILINE',
    'PRECOMP_MACRO',
    'PRECOMP_MACRO_CONT', 
    'ASTERISK',
    'EQUALS',
    'ARRAY_SIZE',
    'BIT_SIZE',
    'NEW_LINE',
    'CONTINUE_LINE',
	'EXTERNC',
]

t_ignore = " \t\r.|:!?%@"
t_NUMBER = r'[\-]{0,1}[0-9][0-9XxA-Fa-f]*'
t_NAME = r'[<>A-Za-z_~][A-Za-z0-9_]*'
t_OPEN_PAREN = r'\('
t_CLOSE_PAREN = r'\)'
t_OPEN_BRACE = r'{'
t_CLOSE_BRACE = r'}'
t_SEMI_COLON = r';'
t_COMMA = r','
t_PRECOMP_MACRO = r'\#.*'
t_PRECOMP_MACRO_CONT = r'.*\\\n'
t_COMMENT_SINGLELINE = r'\/\/.*\n'
t_ASTERISK = r'\*'
t_EQUALS = r'='
t_COMMENT_MULTILINE = r'/\*([^*]|[\r\n]|(\*+([^*/]|[\r\n])))*\*+/'
t_ARRAY_SIZE = r'\[.*\]'
t_BIT_SIZE = r':[0-9]*'
t_NEW_LINE= r'\n+'
t_CONTINUE_LINE= r'\\'
t_EXTERNC = r'extern[\s\t]*[\"]C[\"][\s\t]*{'

def t_error(v):
    print "Lex error: ", v

lex.lex()
debug = False

def is_struct(nameStack):
    """Determines if a struct is being specified"""
    if len(nameStack) == 0:
        return False
    if nameStack[0] == "struct":
        return True
    if len(nameStack) > 1 and nameStack[0] == "typedef" and nameStack[1] == "struct":
        return True
    return False

def is_enum(nameStack):
    """Determines if an  enum namestack"""
    if len(nameStack) == 0:
        return False
    if nameStack[0] == "enum":
        return True
    if len(nameStack) > 1 and nameStack[0] == "typedef" and nameStack[1] == "enum":
        return True
    return False

def is_union(nameStack):
    """Determines if an union namestack"""
    if len(nameStack) == 0:
        return False
    if nameStack[0] == "union":
        return True
    if len(nameStack) > 1 and nameStack[0] == "typedef" and nameStack[1] == "union":
        return True
    return False


def format_comments(comments):
    array = comments.split("*")
    first_Line =True 
    cmn_str = ""
    for c in array :
        c =  c.lstrip().rstrip()
        c =  c.replace(os.linesep, "")
        if first_Line:
            cmn_str +=  c
            first_Line = False
        else:
            cmn_str +="\\n   "
            cmn_str +="          "
            cmn_str +="          "
            cmn_str += c
    return cmn_str

class CMacro:
    def __init__(self, macro):
        self.name = ""
        self.value = ""
        macro.strip()
        #currently, we only process the #define
        if not macro.startswith("#define"):
            return
        tmpList=macro.split()
        if len(tmpList) < 3 :
            return;
        if tmpList[1].find("MASK") != -1:
            #print "-----------------------------------------------"
            #print tmpList[1]
            #print "-----------------------------------------------"
            return
        self.name = tmpList[1]
        self.value = "".join(tmpList[2:])
        if debug :
            print self.name," = ",self.value
            print ""
            print ""

class CEnum: #currently, we only get the enum name
    def __init__(self,nameStack):
        if len(nameStack) < 4 or "{" not in nameStack or "}" not in nameStack:
            #Not enough stuff for an enum
            return
        self.name = ""
        #get enum name 
        preBraceStack = nameStack[:nameStack.index("{")]
        postBraceStack = nameStack[nameStack.index("}") + 1:]
        if (len(preBraceStack) == 2 and "typedef" not in nameStack):
            self.name =  preBraceStack[1]           
        elif len(postBraceStack) and "typedef" in nameStack:
            self.name =  " ".join(postBraceStack)

class CUnion:
    def __init__(self,nameStack):
        if len(nameStack) < 4 or "{" not in nameStack or "}" not in nameStack:
            #Not enough stuff for an enum
            return
        self.fields = []
        self.name = ""

        #Figure out what values it has
        valueStack = nameStack[nameStack.index('{') + 1: nameStack.index('}')]
		
        while len(valueStack):
            #skip the comment at start of union define 
            if valueStack[0].startswith("/*"):
                valueStack = valueStack[1:]
                continue

            tmpStack = []
            field = [] 
            if ";" in valueStack:
                tmpStack = valueStack[:valueStack.index(";")]
                valueStack = valueStack[valueStack.index(";") + 1:]
                if (len(valueStack) != 0 and  valueStack[0].startswith('/*') ):
                    tmpStack.append(valueStack[0]);
                    valueStack = valueStack[1:];
            else:
                tmpStack = valueStack
                valueStack = []

            if(len(tmpStack) < 2 ):
                print "union field define error "
                pass

            field.append(tmpStack[0]);
            field.append(tmpStack[1]);

            if(len(tmpStack) == 3):
                if(tmpStack[2].startswith('/*')) :
                    field.append(0);
                    field.append(tmpStack[2].replace("/*","").replace("*/",""));
                elif(tmpStack[2].startswith('[')) :
                    field.append(tmpStack[2].replace("[","").replace("]",""));
                    field.append("");
            elif(len(tmpStack) == 4):
                field.append(tmpStack[2].replace("[","").replace("]",""));
                field.append(tmpStack[3].replace("/*","").replace("*/",""));
            else:
                field.append(0);
                field.append("");
            #format the comments
            field[3] = format_comments(field[3])

            self.fields.append(field)
        #get struct name 
        preBraceStack = nameStack[:nameStack.index("{")]
        postBraceStack = nameStack[nameStack.index("}") + 1:]
        if (len(preBraceStack) == 2 and "typedef" not in nameStack):
            self.name =  preBraceStack[1]           
        elif len(postBraceStack) and "typedef" in nameStack:
            self.name =  " ".join(postBraceStack)
        if debug :
            print "---------------------------------------"
            print "union:"+ self.name
            print "---------------------------------------"
            print self.fields
            print ""
            print ""



class CStruct:
    def __init__(self, nameStack):
        if len(nameStack) < 4 or "{" not in nameStack or "}" not in nameStack:
            #Not enough stuff for an enum
            return
        self.fields = []
        self.name = ""
        #Figure out what values it has
        valueStack = nameStack[nameStack.index('{') + 1: nameStack.index('}')]

        while len(valueStack):
            #skip the comment at start of struct define 
            if valueStack[0].startswith("/*"):
                valueStack = valueStack[1:]
                continue

            tmpStack = []
            field = [] 
            if ";" in valueStack:
                tmpStack = valueStack[:valueStack.index(";")]
                valueStack = valueStack[valueStack.index(";") + 1:]
                if (len(valueStack) != 0 and  valueStack[0].startswith('/*') ):
                    tmpStack.append(valueStack[0]);
                    valueStack = valueStack[1:];
            else:
                tmpStack = valueStack
                valueStack = []

            if(len(tmpStack) < 2 ):
                print "struct field define error "
                pass

            field.append(tmpStack[0]);
            field.append(tmpStack[1]);

            if(len(tmpStack) == 3):
                if(tmpStack[2].startswith('/*')) :
                    field.append(0);
                    field.append(tmpStack[2].replace("/*","").replace("*/",""));
                elif(tmpStack[2].startswith('[')) :
                    field.append(tmpStack[2].replace("[","").replace("]",""));
                    field.append("");
            elif(len(tmpStack) == 4):
                field.append(tmpStack[2].replace("[","").replace("]",""));
                field.append(tmpStack[3].replace("/*","").replace("*/",""));
            else:
                field.append(0);
                field.append("");
            #format the comments
            field[3] = format_comments(field[3])
            
            union_info = {}
            union_info["union"] = False
            self.paraser_union(union_info,field[3]);
            field.append(union_info);

            self.fields.append(field)
        #get struct name 
        preBraceStack = nameStack[:nameStack.index("{")]
        postBraceStack = nameStack[nameStack.index("}") + 1:]
        if (len(preBraceStack) == 2 and "typedef" not in nameStack):
            self.name =  preBraceStack[1]           
        elif len(postBraceStack) and "typedef" in nameStack:
            self.name =  " ".join(postBraceStack)
        if debug :
            print "---------------------------------------"
            print "struct:"+ self.name
            print "---------------------------------------"
            print self.fields
            print ""
            print ""

    def paraser_union(self,union_info,comments):
        if comments.startswith("[union"):
            union_info["union"] = True
            infos = comments.split("]")
            for info in infos:
                info=info.lstrip().rstrip()
                if info == "" or not info.startswith("["):
                    continue
                info = info.replace("[","")
                field = info.split(":")
                if len(field) < 2:
                    print "WARNING:" +  "".join(field) + " in union descript error" 
                    continue
                field[0] = field[0].lstrip().rstrip()
                field[1] = field[1].lstrip().rstrip()
                if field[0] == "union":
                    union_info["_union_type_"] = field[1]
                else:
                    union_info[field[0]] = field[1].split(",")

class CMethod:
    def __init__(self, nameStack):
        self.name = ""
        if len(nameStack) < 4 or "(" not in nameStack or ")" not in nameStack:
            #Not enough stuff for an enum
            return
        self.params = []
        self.name = ""
        self.ret_type = ""
        decoration = [ "CA_IN", "CA_OUT", "CA_IN_OUT","const","INLINE"]

        for dec in decoration:
            while (dec in nameStack ):
                nameStack.remove(dec)
       
        self.ret_type = " ".join(nameStack[:nameStack.index('(') - 1])
        self.name = " ".join(nameStack[nameStack.index('(') - 1:nameStack.index('(')])

        paramsStack = nameStack[nameStack.index('(') + 1: ]
        while (paramsStack[-1]  != ")"):
            paramsStack.pop()
        paramsStack.pop()

        if len(paramsStack) == 0:
            return

        #no parameters 
        if len(paramsStack) == 1 and paramsStack[0] == "void" :
            return
        
        #igore the function
        #if paramsStack[0].endswith("*/"): 
        #    self.name = ""
        #    return
		
        while len(paramsStack):
            #skip the comment at start of struct define 
            if paramsStack[0].startswith("/*"):
                paramsStack = paramsStack[1:]
                continue

            parm = [] # [type, asterisk,name, desc]
            if "," in paramsStack:
                tmpStack = paramsStack[:paramsStack.index(",")]
                paramsStack = paramsStack[paramsStack.index(",") + 1:]
                if (len(paramsStack) != 0 and  paramsStack[0].startswith('/*') ):
                    tmpStack.append(paramsStack[0]);
                    paramsStack= paramsStack[1:];
            else:
                tmpStack = paramsStack 
                paramsStack = []
            """tmpStack has the following situation
               1. type name 
               2. type name /*comments*/
               3. type * name
               4. type * name /* comments */
            """
            if len(tmpStack) < 2 or ( len(tmpStack) < 3 and  "*" in tmpStack ) :
                print "ERROR: function ",self.name + " param " , tmpStack , " declare error" 
                sys.exit(1)
                
            parm.append(tmpStack[0]);
            if "*" in tmpStack: 
                parm.append(tmpStack[1])
                parm.append(tmpStack[2])
                if len(tmpStack) == 4:
                    parm.append(tmpStack[3].replace("/*","").replace("*/",""))
                else:
                    parm.append("")
 
            else:  
                parm.append("")
                parm.append(tmpStack[1])
                if len(tmpStack) == 3 : 
                    parm.append(tmpStack[2].replace("/*","").replace("*/",""))
                else:
                    parm.append("")
            
            parm[3] = format_comments(parm[3])
            self.params.append(parm)
        if debug  :
            print "==============================="
            print "function:",self.name
            print "return type:",self.ret_type
            print "params:",self.params
            print "==============================="

class CHeader:
    def __init__(self,headerFileName):
        self.headerFileName = os.path.expandvars(headerFileName)
        headerFileStr = ""

        if (len(self.headerFileName)):
            headerFileStr = "\n".join(open(self.headerFileName).readlines())
        
        self.name = os.path.basename(headerFileName).replace("aal_","").replace(".h","");
        
        self.enums = {}
        self.structs = {}
        self.methods = {}
        self.macros = {}
        self.unions = {}

        self.nameStack = []
        self.braceDepth  = 0
        headerFileStr = headerFileStr.replace("__attribute__((packed))","");
        lex.input(headerFileStr)
        while True:
            tok = lex.token()
            if not tok: break      # No more input
            if (tok.type == 'OPEN_BRACE'):
                self.nameStack.append(tok.value)
                self.braceDepth += 1
            elif (tok.type == 'CLOSE_BRACE'):
                self.braceDepth -= 1
                self.nameStack.append(tok.value)
            if (tok.type == 'OPEN_PAREN'):
                self.nameStack.append(tok.value)
            elif (tok.type == 'CLOSE_PAREN'):
                self.nameStack.append(tok.value)
            elif (tok.type == 'EQUALS'):
                self.nameStack.append(tok.value)
            elif (tok.type == 'COMMA'):
                self.nameStack.append(tok.value)
            elif (tok.type == 'NUMBER'):
                continue
            elif (tok.type == 'NAME' or tok.type == 'ASTERISK'):
                self.nameStack.append(tok.value)
            elif (tok.type == 'COLON'):
                self.nameStack.append(tok.value)
            elif (tok.type == 'ARRAY_SIZE'):
                if len(self.nameStack) == 0:
                    continue
                self.nameStack.append(tok.value)
            elif (tok.type == 'COMMENT_SINGLELINE'):
                if len(self.nameStack) == 0:
                    continue
                self.nameStack.append(tok.value)
            elif (tok.type == 'COMMENT_MULTILINE'):
                if len(self.nameStack) == 0:
                    continue
                self.nameStack.append(tok.value)
            elif (tok.type == 'PRECOMP_MACRO'):
                self.evaluate_macro(tok.value)
            elif (tok.type == 'SEMI_COLON'):
                if (self.braceDepth == 0 and len(self.nameStack) != 0 ):
                    self.evaluate_stack();
                else:
                    self.nameStack.append(tok.value)
            elif (tok.type == 'CONTINUE_LINE'):
                self.nameStack.append(tok.value)
				
    def evaluate_stack(self):
        if (len(self.nameStack) == 0):
            return
        #current, we ignore the union
        if( is_union(self.nameStack)):
            #newUnion = CUnion(self.nameStack)
            #if newUnion.name != "":
            #    self.unions[newUnion.name] = newUnion
            self.nameStack = []
            return
        if( is_enum(self.nameStack) ):
            #newEnum = CEnum(self.nameStack)
            #if newEnum.name != "":
            #    self.enums[newEnum.name] = newEnum
            self.nameStack = []
            return
        elif( is_struct(self.nameStack)):
            #newStruct = CStruct(self.nameStack)
            #if newStruct.name != "":
            #    self.structs[newStruct.name] = newStruct
            self.nameStack = []
        else :
            if(self.nameStack[0] != "typedef"  and self.nameStack[0] != "do"):
                newMethod = CMethod(self.nameStack)
                if newMethod.name != "":
                    self.methods[newMethod.name] = newMethod
                self.nameStack = []
            else:
                self.nameStack = []


    def evaluate_macro(self, macro):
        newMacro = CMacro(macro)
        if newMacro.name != "" :
            self.macros[newMacro.name] = newMacro
        
#====================Code gen ====================================

ignore_funcs = [
        "ca_pkt_rx_reg",
        "ca_evt_reg"
        ]

class CodeFile:
    def __init__(self,hfile):
        self.header = CHeader(hfile)

    def get_ko_exports(self):
        gen_str = ""
        for func_name in self.header.methods.keys():
            if func_name in ignore_funcs:
                continue
            gen_str += "EXPORT_SYMBOL(" + func_name + ");\n"
        return gen_str

def main():
    if len (sys.argv) < 2:
        print sys.argv[0] + "<export_file> <handler hfile>  <header file1>   "
        return
    
    out = sys.argv[1]
    header = sys.argv[2:]
    
    if len(header) == 0 :
        return

    dirs = [os.path.dirname(out)]
    for odir in dirs:
        if not os.path.exists(odir) :
            os.mkdir(odir)
    
    inc = ""
    apis = ""
    for hfile in header:
        codeFile = CodeFile(hfile)
        inc += "#include \"" + os.path.basename(hfile) + "\"\n"
        apis += codeFile.get_ko_exports();
    inc += "#include <linux/module.h>\n"
    exFile = open(out,"w");
    exFile.write(inc + apis)
    exFile.flush()
    exFile.close()


if __name__ == "__main__":
    main()
