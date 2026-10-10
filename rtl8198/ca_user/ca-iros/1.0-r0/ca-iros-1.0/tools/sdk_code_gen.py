import ply.lex as lex
import os
import sys
import re
import copy
from shutil import copyfile

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

def is_fpointer(nameStack):
    """Determines if an fpointer namestack"""
    if len(nameStack) == 0:
        return False
    if len(nameStack) > 1 and nameStack[0] == "typedef" and nameStack[2] == "(":
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
            return
        self.name = tmpList[1]
        self.value = "".join(tmpList[2:])
        if debug :
            print self.name," = ",self.value
            print ""
            print ""

class CMacroIfdef:
    def __init__(self,macro):
        self.name = ""
        self.is_end = False
        self.ndef = False
        self.first_ndef = False
        self.is_else = False
        macro.strip() 
        if macro.startswith("#ifdef"):
            self.name = macro.split()[1]
        if macro.startswith("#if defined"):
            self.name = macro.split()[1]
        if macro.startswith("#ifndef"):
            self.name = macro.split()[1]
            self.ndef = True
        if macro.startswith("#endif"):
            self.is_end = True
        if macro.startswith("#else"):
            self.is_else = True

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
    def __init__(self,nameStack,defs):
        if len(nameStack) < 4 or "{" not in nameStack or "}" not in nameStack:
            #Not enough stuff for an union
            return
        self.fields = []
        self.name = ""
        self.defs = defs[:]

        #Figure out what values it has
        valueStack = nameStack[nameStack.index('{') + 1: nameStack.index('}')]

        #skip the ignore tag
        if valueStack[0].startswith("/*"):
            ignore_tag  = valueStack[0].replace("/*","").replace("*/","")
            ignore_tag = ignore_tag.lstrip().rstrip()
            if ignore_tag == "CODEGEN_IGNORE_TAG":
                return

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
            if(tmpStack[1] != "*"):
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
            else:
                field.append(tmpStack[2]);
                if(len(tmpStack) == 4):
                    if(tmpStack[3].startswith('/*')) :
                        field.append(-1);
                        field.append(tmpStack[3].replace("/*","").replace("*/",""));
                else:
                    field.append(-1);
                    field.append("");
                #format the comments
                field[3] = format_comments(field[3])
                self.fields.append(field)
        
        #get union name 
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
    def __init__(self,nameStack,defs):
        if len(nameStack) < 4 or "{" not in nameStack or "}" not in nameStack:
            #Not enough stuff for an struct
            return
        self.fields = []
        self.name = ""
        self.defs = defs[:]
        self.subNameStack = []

        #Figure out what values it has
        # if have sub struct value
        subdef_info = {}
        subdef_info["union"] = False
        subdef_info["struct"] = False
        
        if nameStack.count('{') == 2 and nameStack.count('}') == 2:
            f1 = self.find_all_index(nameStack,'{')
            f2 = self.find_all_index(nameStack,'}')
            #get struct name part1:
            preBraceStack = nameStack[:f1[0]]
            postBraceStack = nameStack[f2[1] + 1:]
            if "union" in nameStack:
                unionvalueStack = nameStack[f1[1] + 1: f2[0]]
                #print unionvalueStack
                unionName = nameStack[f2[0] + 1]
                #print unionName
                valueStack = nameStack[f1[0] + 1: f1[1] - 1] + nameStack[f1[1] + 1: f2[0]] + nameStack[f2[0] + 3: f2[1]]
                #print valueStack
                subdef_info["union"] = True
            elif "struct" in nameStack:
                structvalueStack = nameStack[f1[1] + 1: f2[0]]
                #print unionvalueStack
                structName = nameStack[f2[0] + 1]
                valueStack = nameStack[f1[0] + 1: f1[1] - 1] + nameStack[f1[1] + 1: f2[0]] + nameStack[f2[0] + 3: f2[1]]
                subdef_info["struct"] = True
            else:
                print "sub struct{...} define error "
                return
            #print valueStack
        elif nameStack.count('{') == 1 and nameStack.count('}') == 1:
            valueStack = nameStack[nameStack.index('{') + 1: nameStack.index('}')]
            #get struct name part1:
            preBraceStack = nameStack[:nameStack.index("{")]
            postBraceStack = nameStack[nameStack.index("}") + 1:]
        else:
            print "struct{...} define error "
            return
        #skip the ignore tag
        if valueStack[0].startswith("/*"):
            ignore_tag  = valueStack[0].replace("/*","").replace("*/","")
            ignore_tag = ignore_tag.lstrip().rstrip()
            if ignore_tag == "CODEGEN_IGNORE_TAG":
                return
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
            if tmpStack[0] == "struct":
                tmpStack[0] = "void";
                tmpStack[1] = tmpStack[2];
                tmpStack[2] = tmpStack[3];
                tmpStack[3] = "";
            if tmpStack[0].endswith("_cb"):
                tmpStack[0] = "void";
                tmpStack[2] = tmpStack[1];
                tmpStack[1] = "*";
            field.append(tmpStack[0]);
            if(tmpStack[1] != "*"):
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
            else:
                field.append(tmpStack[2]);
                if(len(tmpStack) == 4):
                    if(tmpStack[3].startswith('/*')) :
                        field.append(-1);
                        field.append(tmpStack[3].replace("/*","").replace("*/",""));
                else:
                    field.append(-1);
                    field.append("");
                #format the comments
                field[3] = format_comments(field[3])
                self.fields.append(field)				
        #get struct name part2: 
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

    def find_all_index(self,arr,item):
        return [i for i,a in enumerate(arr) if a==item]

    def paraser_sub_def(self,subNameStack,subdefs):
        if len(subNameStack) < 4 or "{" not in subNameStack or "}" not in subNameStack:
            #Not enough stuff for an struct
            return
        subfields = []
        subname = ""
        subdefs = subdefs[:]
        
        subValueStack = subNameStack[subNameStack.index('{') + 1: subNameStack.index('}')]
        subpreBraceStack = subNameStack[:subNameStack.index("{")]
        subpostBraceStack = subNameStack[subNameStack.index("}") + 1:]
        
        while len(subValueStack):
            #skip the comment at start of struct define 
            if subValueStack[0].startswith("/*"):
                subValueStack = subValueStack[1:]
                continue

            subTmpStack = []
            subfield = [] 
            if ";" in subValueStack:
                subTmpStack = subValueStack[:subValueStack.index(";")]
                subValueStack = subValueStack[subValueStack.index(";") + 1:]
                if (len(subValueStack) != 0 and  subValueStack[0].startswith('/*') ):
                    subTmpStack.append(subValueStack[0]);
                    subValueStack = subValueStack[1:];
            else:
                subTmpStack = subValueStack
                subValueStack = []

            if(len(subTmpStack) < 2 ):
                print "sub def subfield define error "
                pass

            subfield.append(subTmpStack[0]);
            subfield.append(subTmpStack[1]);

            if(len(subTmpStack) == 3):
                if(subTmpStack[2].startswith('/*')) :
                    subfield.append(0);
                    subfield.append(subTmpStack[2].replace("/*","").replace("*/",""));
                elif(subTmpStack[2].startswith('[')) :
                    subfield.append(subTmpStack[2].replace("[","").replace("]",""));
                    subfield.append("");
            elif(len(subTmpStack) == 4):
                subfield.append(subTmpStack[2].replace("[","").replace("]",""));
                subfield.append(subTmpStack[3].replace("/*","").replace("*/",""));
            else:
                subfield.append(0);
                subfield.append("");
            #format the comments
            subfield[3] = format_comments(subfield[3])
            subfields.append(subfield)
        #print "sub def:"+ subname
        #print subfields   
        if (len(subpreBraceStack) == 2 and "typedef" not in subNameStack):
            subname =  subpreBraceStack[1]           
        elif len(subpostBraceStack):
            subname =  " ".join(subpostBraceStack)
        if debug :
            print "---------------------------------------"
            print "sub def:"+ subname
            print "---------------------------------------"
            print subfields
            print ""
            print ""

        
class CFpointer:
    def __init__(self,nameStack,defs):
        if len(nameStack) < 4 or nameStack.count('(') != 2 or nameStack.count(')') != 2:
            #Not enough stuff for an function pointer
            return
        self.params = []
        self.name = ""
        self.ret_type = ""
        self.defs = defs[:]
        decoration = [ "const"]
        
        for dec in decoration:
            while (dec in nameStack ):
                nameStack.remove(dec) 
        
        f1 = self.find_all_index(nameStack,'(')
        f2 = self.find_all_index(nameStack,')')
        self.ret_type = " ".join(nameStack[f1[0] - 1:f1[0]])
        self.name = " ".join(nameStack[f2[0] - 1:f2[0]])
            
        paramsStack = nameStack[f1[1] + 1:]
        while (paramsStack[-1]  != ")"):
            paramsStack.pop()
        paramsStack.pop()
            
        #no parameters 
        if len(paramsStack) == 1 and paramsStack[0] == "void" :
            return
            
        #igore the function
        if paramsStack[0].startswith("/*") and paramsStack[0].endswith("*/"):
            ignore_tag  = paramsStack[0].replace("/*","").replace("*/","")
            ignore_tag = ignore_tag.lstrip().rstrip()
            if ignore_tag == "CODEGEN_IGNORE_TAG":
                self.name = ""
                return
        
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

            if len(tmpStack) < 2 or ( len(tmpStack) < 3 and  "*" in tmpStack ) :
                print "ERROR: function ",self.name + " param " , tmpStack , " declare error" 
                sys.exit(1)
            
            in_out = [ "CA_IN", "CA_OUT", "CA_IN_OUT"]

            if tmpStack[0] in in_out:
                print "ERROR: function ",self.name + " param " , tmpStack , " declare error, found CA_IN/OUT" 
                sys.exit(1)
            
            parm.append(tmpStack[0]);
            if "*" in tmpStack: 
                parm.append(tmpStack[1])
                parm.append(tmpStack[2])
                if len(tmpStack) == 4:
                    parm.append(tmpStack[4].replace("/*","").replace("*/",""))
                else:
                    parm.append("")
            else:  
                parm.append("")
                parm.append(tmpStack[1])
                if len(tmpStack) == 3: 
                    parm.append(tmpStack[2].replace("/*","").replace("*/",""))
                else:
                    parm.append("")
            parm[3] = parm[3].lstrip().rstrip()
            self.params.append(parm)
        #debug = 1
        if debug  :
            print "----------------------------------"
            print "function:",self.name
            print "return type:",self.ret_type
            print "params:",self.params
            print ""
            print ""

    def find_all_index(self,arr,item):
        return [i for i,a in enumerate(arr) if a == item]

class CMethod:
    def __init__(self, nameStack,defs):
        self.name = ""
        if len(nameStack) < 4 or "(" not in nameStack or ")" not in nameStack:
            #Not enough stuff for an enum
            return
        self.params = []
        self.name = ""
        self.ret_type = ""
        self.defs = defs[:]
        #decoration = [ "CA_IN", "CA_OUT", "CA_IN_OUT","const"]
        decoration = [ "const"]

        for dec in decoration:
            while (dec in nameStack ):
                nameStack.remove(dec)
       
        self.ret_type = "".join(nameStack[:nameStack.index('(') - 1]).replace("extern","")
        self.name = " ".join(nameStack[nameStack.index('(') - 1:nameStack.index('(')])

        paramsStack = nameStack[nameStack.index('(') + 1:]
        while (paramsStack[-1]  != ")"):
            paramsStack.pop()
        paramsStack.pop()

        #no parameters 
        if len(paramsStack) == 1 and paramsStack[0] == "void" :
            return
        
        #igore the function
        if paramsStack[0].startswith("/*") and paramsStack[0].endswith("*/"):
            ignore_tag  = paramsStack[0].replace("/*","").replace("*/","")
            ignore_tag = ignore_tag.lstrip().rstrip()
            if ignore_tag == "CODEGEN_IGNORE_TAG":
                self.name = ""
                return

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
               1. CA_IN/CA_OUT type name 
               2. CA_IN/CA_OUT type name /*comments*/
               3. CA_IN/CA_OUT type * name
               4. CA_IN/CA_OUT type * name /* comments */
            """
            if len(tmpStack) < 3 or ( len(tmpStack) < 4 and  "*" in tmpStack ) :
                print "ERROR: function ",self.name + " param " , tmpStack , " declare error" 
                sys.exit(1)
                

            in_out = [ "CA_IN", "CA_OUT", "CA_IN_OUT"]

            if tmpStack[0] not in in_out:
                print "ERROR: function ",self.name + " param " , tmpStack , " declare error, not found CA_IN/OUT" 
                sys.exit(1)

            parm.append(tmpStack[1]);
            if "*" in tmpStack: 
                parm.append(tmpStack[2])
                parm.append(tmpStack[3])
                if len(tmpStack) == 5:
                    parm.append(tmpStack[4].replace("/*","").replace("*/",""))
                else:
                    parm.append("")
            else:  
                parm.append("")
                parm.append(tmpStack[2])
                if len(tmpStack) == 4 : 
                    parm.append(tmpStack[3].replace("/*","").replace("*/",""))
                else:
                    parm.append("")
            parm[3] = parm[3].lstrip().rstrip()
            #parm[3] = format_comments(parm[3])
            parm.append(tmpStack[0])
            size = 0
            cnt = ""
            if parm[3].startswith("[") and parm[3].endswith("]"):
                parm[3] = parm[3].replace("[","").replace("]","")
                array=parm[3].split(":");
                if len(array) == 2:
                    size = int(array[1])
                    cnt = array[0]
                else:
                    if parm[1] != "*":
                        parm[1] = "*"
                    else:
                        parm[1] = "**"

            parm.append(size)
            parm.append(cnt)

            self.params.append(parm)
        if debug  :
            print "----------------------------------"
            print "function:",self.name
            print "return type:",self.ret_type
            print "params:",self.params
            print ""
            print ""


class CHeader:
    def __init__(self,headerFileName):
        self.headerFileName = os.path.expandvars(headerFileName)
        headerFileStr = ""

        if (len(self.headerFileName)):
            headerFileStr = "\n".join(open(self.headerFileName).readlines())
        
        self.name = os.path.basename(headerFileName).replace("aal_","").replace(".h","");
        
        self.enums = {}
        self.structs = {}
        self.fpointers = {}
        self.methods = {}
        self.macros = {}
        self.unions = {}
        self.ifdefs = []
        self.ndef = True

        self.nameStack = []
        self.braceDepth  = 0
        headerFileStr = headerFileStr.replace("__attribute__((packed))","");
        headerFileStr = headerFileStr.replace("_CA_STRUCT_ALIGN_","");
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

    def evaluate_stack(self):
        if (len(self.nameStack) == 0):
            return
        #current, we ignore the union
        if( is_union(self.nameStack)):
            newUnion = CUnion(self.nameStack,self.ifdefs)
            if newUnion.name != "":
                self.unions[newUnion.name] = newUnion
            self.nameStack = []
            return
        if( is_enum(self.nameStack) ):
            newEnum = CEnum(self.nameStack)
            if newEnum.name != "":
                self.enums[newEnum.name] = newEnum
            self.nameStack = []
            return
        elif( is_struct(self.nameStack)):
            newStruct = CStruct(self.nameStack,self.ifdefs)
            if newStruct.name != "":
                self.structs[newStruct.name] = newStruct
            self.nameStack = []
        elif( is_fpointer(self.nameStack)):
            newFpointer = CFpointer(self.nameStack,self.ifdefs)
            if newFpointer.name != "":
                self.fpointers[newFpointer.name] = newFpointer
            self.nameStack = []
        else :
            newMethod = CMethod(self.nameStack,self.ifdefs)
            if newMethod.name != "":
                self.methods[newMethod.name] = newMethod
            self.nameStack = []

    def evaluate_macro(self, macro):
        newMacro = CMacro(macro)
        if newMacro.name != "" :
            self.macros[newMacro.name] = newMacro
        else:
            ifdefs= CMacroIfdef(macro)
            if ifdefs.name != "":
                #skip the proctected macro
                if ifdefs.ndef and self.ndef:
                    self.ndef = False
                    ifdefs.first_ndef = True

                self.ifdefs.append(ifdefs)
            if ifdefs.is_end and len(self.ifdefs) > 0 :
                self.ifdefs.pop()
            if ifdefs.is_else:
                item = copy.deepcopy(self.ifdefs.pop())
                item.ndef = not item.ndef;
                self.ifdefs.append(item)

#====================Code gen ====================================
ca_basic_types = [
    "ca_boolean_t",
    "ca_uint8_t",
    "ca_char8_t",
    "ca_uchar8_t",
    "ca_int8_t",
    "ca_int16_t",
    "ca_int32_t",
    "ca_int64_t",
    "ca_uint16_t",
    "ca_uint32_t",
    "ca_uint64_t",
    "ca_device_id_t",
    "ca_port_id_t",
    "ca_vlan_id_t",
    "ca_mpcp_llid_t",
    "ca_port_bmp_t",
    "ca_pon_tunnel_id_t",
    "ca_epon_mpcp_report_mode_t",
    "ca_status_t",
    "ca_intf_id_t",
    "ca_tunnel_id_t",
    "ca_l3_nexthop_id_t",
    "ca_session_id_t",
    "ca_llid_instance_t",
    "ca_flow_id_t",
    "ca_ipv4_addr_t",
    "ca_boolean",
    "ca_addr",
    "ca_uint8",
    "ca_int8",
    "ca_uint16",
    "ca_int16",
    "ca_uint32",
    "ca_int32",
    "ca_uint64",
    "ca_int64",
    "ca_double",
    "ca_char",
    "ca_llid_t",
    "ca_sub_id_t",
    "ca_rtp_id_t",
    "ca_ip_translate_id_t",
    "ca_ipsec_sa_id_t",
    "ca_pptp_sa_id_t",
    "ca_macsec_sa_id_t",
    "void"
    ]

ca_bitfield_types = [
    "ca_vlan_key_type_t",
    "ca_macsec_global_config_t",
    "ca_sadb_flags_t",
    "ca_macsec_sa_db_mask_t",
    ]

ca_buildin_enums = [
    "ca_tunnel_type_t",
    "ca_ip_afi_t",
    "ca_l3_intf_type_t",
    "ca_encap_type_t"
    "ca_qos_mode_t",
    "ca_policer_mode_t",
    "ca_port_encap_type_t",
    "ca_pm_state_t",
    "ca_cpu_freq_t",
    "ca_classifier_forward_flag_t",
    "ca_pkt_type_t",
    "ca_classifier_vlan_action_t",
    "ca_epon_port_fec_stat_t",
    "ca_vlan_tag_type_t",
    "ca_vlan_tag_action_t",
    "ca_vlan_tpid_source_t",
    "ca_vlan_new_vlan_source_t",
    "ca_vlan_tag_priority_source_t",
    "ca_vlan_learning_mode_t",
    "ca_vlan_ing_port_type_t",
    "ca_vlan_action_direction_t"
    ]

ca_buildin_structs = [
    "ca_ipv6_t",
    "ca_classifier_handle_t",
    "ca_llid_bmp_t",
    "ca_psg_llid_t",
    "ca_llid_cfg_t",
    "ca_policer_t",
    "ca_llid_stats_t",
    "ca_funcptr",
    "ca_l3_ip_addr_t",
    "ca_ip_address_t",
    "ca_tunnel_cfg_t",
    "ca_ipsec_tunnel_cfg_t",
    "ca_ipsec_sa_t",
    "ca_ip_translate_cfg_t",
    "ca_flow_t",
    "ca_classifier_key_t",
    "ca_classifier_action_t",
    "ca_event_app_context_info_t",
    "ca_event_cfg_s",
    "ca_pkt_block_t",
    "ca_pkt_t",
    "ca_pkt_s",
    "ca_l3_route_t",
    "ca_l3_intf_t",
    "ca_l3_nexthop_t",
    "ca_classifier_ext_t",
    "ca_iterator_t",
    "L3FE_HDR_I_t",
    "aal_hash_key_t",
    "aal_hash_mask_t",
    "aal_hash_action_t",
    "aal_hash_tuple_type_e",
    "aal_hashlite_aging_e",
    "lpm_tbl_entry_t",
    "lpm_action_ctrl_t",
    "l3pe_dsl_tbl_entry_t",
    "l3pe_sixrd_tbl_entry_t",
    "ca_l2_mac_filter_entry_t",
    "ca_classifier_key_mask_t",
    "ca_l2_addr_entry_t",
    "ca_l2_mcast_entry_t",
    "ca_vlan_key_type_t",
    "ca_macsec_global_config_t",
    "ca_sadb_flags_t",
    "ca_macsec_sa_db_mask_t",
    ]

ca_buildin_arrays = [
    "ca_mac_addr_t"
    ]

ca_buildin_fpointers = [
    "ca_classifier_rule_traverse_cb",
    "ca_l2_addr_filter_traverse_cb",
    "ca_l2_addr_traverse_cb",
    "ca_l2_mcast_member_traverse_cb",
    "ca_l3_mcast_member_traverse_cb",
    "ca_nat_entry_traverse_cb",
    "ca_l3_intf_traverse_cb",
    "ca_l3_route_traverse_cb",
    "ca_l3_nexthop_traverse_cb",
    "ca_pkt_block_cb",
    "ca_rx_cb",
    "ca_tunnel_traverse_cb",
    "ca_ipsec_sa_traverse_cb",
    "ca_l2_vlan_traverse_cb"
    ]
    
ca_defs = ca_basic_types +  ca_buildin_structs + ca_buildin_enums + ca_buildin_arrays

ca_types2_dicts = {
    "ca_boolean_t":"unsigned int",
    "ca_uint8_t":"unsigned char",
    "ca_uint16_t":"unsigned short",
    "ca_int16_t":"short",
    "ca_int8_t":"char",
    "ca_uint32_t":"unsigned int",
    "ca_int32_t":"int",
    "ca_uint64_t":"unsigned long long",
    "ca_int64_t":"long long",
}

ca_types_dicts = {
    "ca_boolean":"unsigned int",
    "ca_addr":"unsigned int",
    "ca_uint8":"unsigned char",
    "ca_uint16":"unsigned short",
    "ca_int16":"short",
    "ca_int8":"char",
    "ca_uint32":"unsigned int",
    "ca_mpcp_id_t":"int",
    "ca_int32":"int",
    "ca_char":"char",
    "ca_uint64":"unsigned long long",
    "ca_int64":"long long ",
    "ca_double":"double",
    "ca_device_id_t":"unsigned short",
    "ca_port_id_t":"unsigned int",
    "ca_mpcp_llid_t":"unsigned short",
    "ca_vlan_id_t":"unsigned int",
    "ca_port_bmp_t":"unsigned int",
    "ca_pon_tunnel_id_t":"unsigned char",
    "ca_llid_t":"unsigned short",
    "ca_sub_id_t":"unsigned short",
    "ca_intf_id_t":"unsigned short",
    "ca_status_t":"unsigned short",
    "ca_tunnel_id_t":"unsigned short",
    "ca_l3_nexthop_id_t":"unsigned short",
    "ca_rtp_id_t":"unsigned int",
    "ca_ip_translate_id_t":"unsigned int",
    "ca_ipsec_sa_id_t":"unsigned int",
    "ca_pptp_sa_id_t":"unsigned int",
    "ca_macsec_sa_id_t":"unsigned int",
    "ca_session_id_t":"unsigned short",
    "ca_llid_instance_t":"unsigned int",
    "ca_flow_id_t":"unsigned int",
    "ca_ipv4_addr_t":"unsigned int",
    "ca_eth_port_speed_t":"unsigned int",
}

ca_types_size_table = {
    "ca_boolean_t":4,
    "ca_boolean":4,
    "ca_uint8_t":1,
    "ca_uint16_t":2,
    "ca_int16_t":2,
    "ca_int8_t":1,
    "ca_char":1,
    "ca_uint32_t":4,
    "ca_int32_t":4,
    "ca_uint64_t":8,
    "ca_int64_t":8,
    "ca_uint8":1,
    "ca_uint16":2,
    "ca_int16":2,
    "ca_int8":1,
    "ca_uint32":4,
    "ca_int32":4,
    "ca_uint64":8,
    "ca_int64":8,
    "ca_double":8,
    "ca_device_id_t":2,
    "ca_port_id_t":4,
    "ca_tunnel_id_t":2,
    "ca_mpcp_llid_t":2,
    "ca_rtp_id_t":4,
    "ca_ip_translate_id_t":4,
    "ca_ipsec_sa_id_t":4,
    "ca_pptp_sa_id_t":4,
    "ca_macsec_sa_id_t":4,
    "ca_session_id_t":2,
    "ca_tunnel_type_t":4,
    "ca_ip_afi_t":4,
    "ca_l3_intf_type_t":4,
    "ca_encap_type_t":4,
    "ca_qos_mode_t":4,
    "ca_policer_mode_t":4,
    "ca_pm_state_t":4,
    "ca_cpu_freq_t":4,
    "ca_vlan_id_t":4,
    "ca_port_bmp_t":4,
    "ca_pon_tunnel_id_t":1,
    "ca_llid_t":4,
    "ca_sub_id_t":4,
    "ca_mac_addr_t":6,
    "ca_intf_id_t":2,
    "ca_l3_nexthop_id_t":2,
    "ca_cpu_freq_t":4,
    "ca_llid_instance_t":4,
    "ca_flow_id_t":4,
    "ca_ipv4_addr_t":4,
    "ca_eth_port_speed_t":4,
	"void":4,
	"ca_pkt_block_cb":4,
    "ca_vlan_key_type_t":4,
    "ca_macsec_global_config_t":4,
    "ca_sadb_flags_t":4,
    "ca_macsec_sa_db_mask_t":4,
}


class FuncString:
    def __init__(self,ret, func_name):
        self.ret_type = ret; 
        self.func_name = func_name;
        self.params = "";
        self.var_defs = "";
        self.proc = "";
        self.defs = []

    def gen_c_code(self):
        ret = ""
        endif=""
        for macro in self.defs:
            if macro.first_ndef:
                continue;
            if macro.ndef:
                ret +="#ifndef " +  macro.name + "\n"
            else:
                ret +="#ifdef " +  macro.name + "\n"
            endif +="#endif \n"
        ret += self.ret_type + " "
        ret += self.func_name + "(" + self.params + ")\n"
        ret += "{\n" 
        if self.var_defs != "":
            ret += self.var_defs + "\n\n"
        ret += self.proc + "\n"
        ret += "}\n"
        ret += endif
        ret +="\n"
        return ret

    def gen_define(self):
        ret = ""
        for macro in self.defs:
            if macro.first_ndef:
                continue;
            if macro.ndef:
                ret +="#ifndef " +  macro.name + "\n"
            else:
                ret +="#ifdef " +  macro.name + "\n"
        ret += "extern " + self.ret_type + " "
        ret += self.func_name + "(" + self.params + ");\n"
        for macro in self.defs:
            if macro.first_ndef:
                continue;
            ret +="#endif  \n"
        return ret; 

class SdkTCLFunction:
    def __init__(self,sdkHeaderFile):
        self.funcs = [];
        self.structs = sdkHeaderFile.structs
        self.unions = sdkHeaderFile.unions
        self.enums = sdkHeaderFile.enums

    def get_tag(self,struct):
        suffix = "_t"
        tag = struct
        if struct.endswith(suffix):
            tag = struct[:-len(suffix)]
        return tag

    def gen_structs(self,structs):
        for struct in structs :
            self.gen_create_func(struct,structs[struct].defs)
            self.gen_init_func(struct,structs[struct].fields,structs[struct].defs)
            self.gen_dump_func(struct,structs[struct].fields,structs[struct].defs)
            self.gen_set_get_func(struct,structs[struct].fields,structs[struct].defs)
            self.gen_array_func(struct,structs[struct].defs)

    def gen_unions(self,unions):
        for union in unions :
            self.gen_create_func(union,unions[union].defs)
            self.gen_dump_func(union,unions[union].fields,unions[union].defs)
            self.gen_set_get_func(union,unions[union].fields,unions[union].defs)
            self.gen_array_func(union,unions[union].defs)

    def gen_create_func(self,data_type,defs):
        tag = self.get_tag(data_type)
        create_func = FuncString(data_type + " * ",  tag + "_create")
        tab = "    "
        create_func.var_defs = tab + data_type + " * cfg = malloc(sizeof(" + data_type + ")); " 
        create_func.proc = tab + "if(cfg != NULL){\n"
        create_func.proc += tab + "    memset(cfg,0x00,sizeof(" + data_type + "));\n"
        create_func.proc += tab + "}\n\n"
        create_func.proc += tab + "return cfg;"
        create_func.defs = defs
        self.funcs.append(create_func)

    def gen_init_func(self,data_type,fields,defs):
        tag = self.get_tag(data_type)
        init_func = FuncString("void",  tag + "_init")
        tab = "    "
        params = data_type + " * cfg"
        proc = ""
        verify = tab + "if(cfg != NULL\n"
        for f in fields:
            if f[2] != 0 and f[2] != -1:
                continue
            if self.structs.has_key(f[0]) or self.unions.has_key(f[0]):
                params +=",\n    " + f[0] + " * " + f[1]
                verify += tab + "    && NULL != " + f[1] + "\n"
                proc += tab + "    memcpy(&cfg->"+ f[1] +","+ f[1] + ",sizeof("+f[0]+"));\n" 
            elif f[2] == -1 and f[0] in ca_basic_types:
                params +=",\n    " + f[0] + " * " + f[1]
                verify += tab + "    && NULL != " + f[1] + "\n"
                proc += tab + "    cfg->"+ f[1] +" =  "+ f[1] + ";\n" 
            elif f[0] in ca_basic_types or f[0] in self.enums.keys() or f[0] in ca_buildin_enums:
                if tag == "ca_classifier_action" and f[1] != "forward":
                    params +=",\n    " + f[0] + " " + f[1]
                    proc += tab + "    if("+ f[1] +")"+ "\n"
                    proc += tab + "    {\n"
                    proc += tab + "        cfg->dest."+ f[1] +" =  "+ f[1] + ";\n"
                    proc += tab + "    }\n"
                else:
                    if f[2] == -1 and f[0] != "ca_ipsec_sa_id_t":
                        params +=",\n    " + f[0] + " * " + f[1]
                        verify += tab + "    && NULL != " + f[1] + "\n"
                        proc += tab + "    cfg->"+ f[1] +" =  &"+ f[1] + ";\n" 
                    else:
                        params +=",\n    " + f[0] + " " + f[1]
                        proc += tab + "    cfg->"+ f[1] +" =  "+ f[1] + ";\n" 
            elif f[0] == "ca_ipv6_t":
                params +=",\n    " + f[0] + " * " + f[1]
                verify += tab + "    && NULL != "  + f[1] + "\n"
                proc += tab + "    memcpy(&cfg->"+ f[1] +"[0],"+ f[1] + ",sizeof("+f[0]+"));\n" 
            elif f[0] == "ca_mac_addr_t":
                params +=",\n    " + f[0] + " * " + f[1]
                verify += tab + "    && NULL != "  + f[1] + "\n"
                proc += tab + "    memcpy(&cfg->"+ f[1] +","+ f[1] + ",sizeof("+f[0]+"));\n" 
            else: #mac, ipv6, llid
                params +=",\n    " + f[0] + " * " + f[1]
                verify += tab + "    && NULL != "  + f[1] + "\n"
                proc += tab + "    memcpy(&cfg->"+ f[1] +","+ f[1] + ",sizeof("+f[0]+"));\n" 
        verify += tab + ")\n"
        verify += tab + "{\n"
        init_func.params = params
        init_func.defs = defs
        init_func.proc = verify + proc + tab + "}\n"
        self.funcs.append(init_func)

    def gen_dump_func(self,data_type,fields,defs):
        tag = self.get_tag(data_type)
        dump_func = FuncString("void",  tag + "_dump")
        tab = "    "
        dump_func.params = data_type + " * cfg"
        dump_func.defs = defs
        proc = ""
        verify = tab + "if(cfg == NULL){\n"
        verify += tab + "    return;\n"
        verify += tab + "}\n\n"
        def_var = 0; #0 - no define, 1 - define i, 2 - define j. 3 - define i and j
        for f in fields:
            if f[2] != 0 and f[2] != -1:
                if  def_var == 0 or def_var == 2:
                    dump_func.var_defs += tab + "ca_uint32 i = 0;\n"
                    if def_var == 0:
                        def_var = 1
                    elif def_var == 2:
                        def_var = 3
                proc +=tab + "for(i = 0; i < "+ f[2] + ";i++){\n"
            
            if f[0] == "ca_mac_addr_t":
                if def_var == 1 or def_var == 0:
                    dump_func.var_defs += tab + "ca_uint32 j = 0;\n"
                    if def_var == 0:
                        def_var = 2
                    elif def_var == 1:
                        def_var = 3
                if f[2] != 0:
                    proc += tab + "    for(j = 0 ; j < 6; j++){ \n"
                    proc += tab + "        printf(\"" + f[1] + "[%d][%d] = 0x%x\\n\",i,j,cfg->" + f[1] + "[i][j]);\n" 
                    proc += tab + "    }\n"
                else:
                    proc += tab + "for(j = 0 ; j < 6; j++){ \n"
                    proc += tab + "    printf(\"" + f[1] + "[%d] = 0x%x\\n\",j,cfg->" + f[1] + "[j]);\n" 
                    proc += tab + "}\n"
            elif f[0] == "ca_l3_addr_t":
                if def_var == 1 or def_var == 0:
                    dump_func.var_defs += tab + "ca_uint32 j = 0;\n"
                    if def_var == 0:
                        def_var = 2
                    elif def_var == 1:
                        def_var = 3
                if f[2] != 0:
                    proc += tab + "    printf(\"" + f[1] + "[%d].ipv4 = %d \\n\",i,cfg->" + f[1]+".ipv4); \n"
                    proc += tab + "    for(j = 0 ; j < 8; j++){ \n"
                    proc += tab + "        printf(\"" + f[1] + ".ipv6[%d][%d] = 0x%x\\n\",i,j,cfg->" + f[1] + "[i].ipv6[j]);\n" 
                    proc += tab + "    }\n"
                else:
                    proc += tab + "printf(\"" + f[1] + ".ipv4 = %d \\n\",cfg->" + f[1]+".ipv4); \n"
                    proc += tab + "for(j = 0 ; j < 8; j++){ \n"
                    proc += tab + "    printf(\"" + f[1] + ".ipv6[%d] = 0x%x\\n\",j,cfg->" + f[1] + ".ipv6[j]);\n" 
                    proc += tab + "}\n"
            elif f[0] == "ca_police_cfg_t":
                if f[2] != 0:
                    proc += tab + "    printf(\"" + f[1] + "[%d].ratio = 0x%x\\n\",i,cfg->" + f[1] + "[i].ratio);\n" 
                    proc += tab + "    printf(\"" + f[1] + "[%d].burst = 0x%x\\n\",i,cfg->" + f[1] + "[i].burst);\n" 
                    proc += tab + "    printf(\"" + f[1] + "[%d].token = 0x%x\\n\",i,cfg->" + f[1] + "[i].token);\n" 
                else:
                    proc += tab + "printf(\"" + f[1] + ".ratio = 0x%x\\n\",cfg->" + f[1] + ".ratio);\n" 
                    proc += tab + "printf(\"" + f[1] + ".burst = 0x%x\\n\",cfg->" + f[1] + ".burst);\n" 
                    proc += tab + "printf(\"" + f[1] + ".token = 0x%x\\n\",cfg->" + f[1] + ".token);\n" 
            elif f[0] == "ca_ipv6_t":
                sub_tag = self.get_tag(f[0])
                if f[2] != 0:
                    proc += tab + "    " + sub_tag + "_dump(cfg->" + f[1] + "[i]);\n"
                else:
                    proc += tab + sub_tag + "_dump(cfg->" + f[1] + ");\n"
            elif self.structs.has_key(f[0]) or self.unions.has_key(f[0]) or f[0] in ca_buildin_structs:
                sub_tag = self.get_tag(f[0])
                if f[2] != 0 and f[2] != -1:
                    proc += tab + "    " + sub_tag + "_dump(&cfg->" + f[1] + "[i]);\n"
                else:
                    if f[1] == "pkt_data":
                        proc += tab + "if(cfg->" + f[1] + " != NULL)\n"
                        proc += tab + "    " + sub_tag + "_dump(cfg->" + f[1] + ");\n"
                    else:
                        proc += tab + sub_tag + "_dump(&cfg->" + f[1] + ");\n"
            elif f[0] in ca_basic_types or self.enums.has_key(f[0]) or f[0] in ca_buildin_enums :
                if f[2] != 0 and f[2] != -1:
                    if f[0] in ["ca_uint64","ca_int64","ca_uint64_t","ca_int64_t"]:
                        proc += tab + "    printf(\"" + f[1] + "[%d] = 0x%llx\\n\",i,cfg->" + f[1] + "[i]);\n" 
                    elif f[0] in ["ca_double","ca_double_t"]:
                        proc += tab + "    printf(\"" + f[1] + "[%d] = %f\\n\",i,cfg->" + f[1] + "[i]);\n" 
                    else:
                        proc += tab + "    printf(\"" + f[1] + "[%d] = 0x%x\\n\",i,cfg->" + f[1] + "[i]);\n" 
                elif f[2] == -1:
                    proc += tab + "if((cfg->" + f[1] + ") != NULL)\n"
                    if f[0] == "void":
                        proc += tab + "    printf(\"" + "&" + f[1] + " = %p\\n\", &(cfg->" + f[1] + "));\n"
                    else:
                        #proc += tab + "    printf(\"" + "*" + f[1] + " = 0x%x\\n\",*(cfg->" + f[1] + "));\n"
                        proc += tab + "    printf(\"" + "&" + f[1] + " = %p\\n\",(cfg->" + f[1] + "));\n"
                else:
                    if f[0] in ["ca_uint64","ca_int64","ca_uint64_t","ca_int64_t"]:
                        proc += tab + "printf(\"" + f[1] + " = 0x%llx \\n\",cfg->" + f[1] + ");\n" 
                    elif f[0] in ["ca_double","ca_double_t"]:
                        proc += tab + "printf(\"" + f[1] + " = %f \\n\",cfg->" + f[1] + ");\n" 
                    else:
                        if tag == "ca_classifier_action" and f[1] != "forward":
                            proc += tab + "printf(\"" + f[1] + " = 0x%x \\n\",cfg->dest." + f[1] + ");\n" 
                        else:
                            proc += tab + "printf(\"" + f[1] + " = 0x%x \\n\",cfg->" + f[1] + ");\n" 
            elif f[0] in ca_buildin_fpointers:
                proc += tab + "if(&(cfg->" + f[1] + ") != NULL)\n"
                proc += tab + "    printf(\"" + "&" + f[1] + " = %p\\n\", &(cfg->" + f[1] + "));\n"
            
            if f[2] != 0 and f[2] != -1:
                proc += tab + "}\n"
        dump_func.proc = verify + proc 
        self.funcs.append(dump_func)

    def gen_array_func(self,data_type,defs):
        tag = self.get_tag(data_type)
        create_func = FuncString(data_type + " * ",  tag + "_array_create")
        get_func = FuncString(data_type + " * ",  tag + "_array_get")
        set_func = FuncString("void ",  tag + "_array_set")
        tab = "    "
        create_func.params = "ca_uint32 size"
        get_func.params = data_type + " * arr,ca_uint32 idx"
        set_func.params = data_type + " * arr," + data_type + " * item,ca_uint32 idx"

        #create function
        create_func.proc = tab + data_type + " * arr = malloc(size * sizeof("+ data_type + "));\n"
        create_func.proc += tab + "if(arr != NULL){\n"
        create_func.proc += tab + "    memset(arr,0x00,size * sizeof(" + data_type + "));\n"
        create_func.proc += tab + "}\n\n"
        create_func.proc += tab + "return arr;"

        #get function
        get_func.proc += tab + "if(arr == NULL){\n"
        get_func.proc += tab + "    return NULL; \n"
        get_func.proc += tab + "} \n\n"
        get_func.proc += tab + "/* need check range by input*/\n"
        get_func.proc += tab + "return &arr[idx]; "

        #set function
        set_func.proc += tab + "if(arr == NULL || item == NULL){\n"
        set_func.proc += tab + "    return ; \n"
        set_func.proc += tab + "} \n\n"
        set_func.proc += tab + "/* need check range by input*/\n"
        set_func.proc += tab + "memcpy(&arr[idx],item,sizeof(" + data_type + "));\n"

        create_func.defs = defs
        set_func.defs = defs
        get_func.defs = defs

        self.funcs.append(create_func)
        self.funcs.append(set_func)
        self.funcs.append(get_func)

    def gen_set_get_func(self,data_type,fields,defs):
        tag = self.get_tag(data_type)
        for f in fields:
            set_func = FuncString("void",  tag + "_set_" + f[1])
            get_params = data_type + " * cfg"
            set_params = get_params
            set_vars = ""
            tab = "    "
            set_proc = tab + "if(cfg == NULL"
            get_proc = set_proc + "){\n"
            if f[0] in ca_bitfield_types :
                get_err_proc = tab + "    return NULL;\n"
            else:
                get_err_proc = tab + "    return 0;\n"
            set_err_proc = tab + "    return ;\n"
            get_err_proc += tab + "}\n\n"
            set_err_proc += tab + "}\n\n"
            if f[0] in ca_basic_types :
                if f[0] == "void":
                    get_func = FuncString("void *",  tag + "_get_" + f[1])
                    set_params += "," + f[0] + " * " + f[1]
                    set_proc += "){\n"
                elif f[2] == -1:
                    get_func = FuncString(f[0] ,  tag + "_get_" + f[1])
                    set_params += "," + f[0] + " * " + f[1]
                    set_proc += "){\n"
                else:
                    get_func = FuncString(f[0] ,  tag + "_get_" + f[1])
                    set_params += "," + f[0] + " " + f[1]
                    set_proc += "){\n"
            elif self.enums.has_key(f[0]) or f[0] in ca_buildin_enums:
                get_func = FuncString(f[0] ,  tag + "_get_" + f[1])
                set_params += "," + f[0] + " " + f[1]
                set_proc += "){\n"
            elif f[0] == "ca_ipv6_t":
                get_func = FuncString("ca_uint16 *" ,  tag + "_get_" + f[1])
                set_params += "," + f[0] + " " + f[1]
                set_proc += "){\n"
            elif f[0] == "ca_mac_addr_t":
                get_func = FuncString(f[0]  + " * ",  tag + "_get_" + f[1])
                set_params += "," + f[0] + " * " + f[1]
                set_proc += " || " + f[1] + " == NULL){ \n"
            elif f[0] == "ca_pkt_block_t":
                get_func = FuncString(f[0]  + " * ",  tag + "_get_" + f[1])
                set_params += "," + f[0] + "* " + f[1]
                set_proc += " || " + f[1] + " == NULL){ \n"
            else:
                get_func = FuncString(f[0]  + " * ",  tag + "_get_" + f[1])
                set_params += "," + f[0] + "* " + f[1]
                set_proc += " || " + f[1] + " == NULL){ \n"
            set_proc += set_err_proc
            get_proc += get_err_proc

            if f[2] != 0 and f[2] != -1:
                set_params += ",ca_uint32 idx"
                get_params += ",ca_uint32 idx"
                set_vars = tab +  "ca_uint32 i = 0;"
                set_proc += tab + "if(idx == 0xffffffff){ \n"
                set_proc += tab + "    for(i = 0 ; i < " + f[2] + ";i++){\n"
                get_proc += tab + "if(idx >= " + f[2] + "){\n"
                get_proc += tab + "    return 0;\n"
                get_proc += tab + "}\n"

                if f[0] in ca_basic_types or  self.enums.has_key(f[0]) or f[0] in ca_buildin_enums:
                    set_proc += tab + "        cfg->" + f[1] + "[i] = " + f[1] + ";\n"
                elif f[0] == "ca_ipv6_t":
                    set_proc += tab + "        memcpy(cfg->" + f[1] + "[i]," + f[1] + ",sizeof("+f[0]+"));\n"
                else:
                    set_proc += tab + "        memcpy(&cfg->" + f[1] + "[i]," + f[1] + ",sizeof("+f[0]+"));\n"
                set_proc += tab + "    }\n"
                set_proc += tab + "}else if(idx < " + f[2] + "){\n"
                if f[0] in ca_basic_types or  self.enums.has_key(f[0]) or f[0] in ca_buildin_enums:
                    get_proc += tab + "return cfg->" + f[1] + "[idx];"
                    set_proc += tab + "    cfg->" + f[1] + "[idx] = " + f[1] + ";\n"
                elif f[0] == "ca_ipv6_t":
                    get_proc += tab + "return cfg->" + f[1] + "[idx];"
                    set_proc += tab + "    memcpy(cfg->" + f[1] + "[idx]," + f[1] + ",sizeof("+f[0]+"));\n"
                else:
                    get_proc += tab + "return &cfg->" + f[1] + "[idx];"
                    set_proc += tab + "    memcpy(&cfg->" + f[1] + "[idx]," + f[1] + ",sizeof("+f[0]+"));\n"
                set_proc += tab + "}else{\n"
                set_proc += tab + "    printf(\"%s bad Index \\n\",__FUNCTION__);\n" 
                set_proc += tab + "}"
            elif f[2] == -1 and f[0] in ca_basic_types:
                set_proc += tab + "cfg->" + f[1] + " = " + f[1] + ";"
                if f[0] == "void":
                    get_proc += tab + "return (void *)(cfg->" + f[1] + ");"
                else:
                    get_proc += tab + "return *(cfg->" + f[1] + ");"
            else:
                if f[0] in ca_basic_types or  self.enums.has_key(f[0]) or f[0] in ca_buildin_enums:
                    if tag == "ca_classifier_action" and f[1] != "forward":
                        set_proc += tab + "cfg->dest." + f[1] + " = " + f[1] + ";"
                        get_proc += tab + "return cfg->dest." + f[1] + ";"
                    else:
                        set_proc += tab + "cfg->" + f[1] + " = " + f[1] + ";"
                        get_proc += tab + "return cfg->" + f[1] + ";"
                elif f[0] == "ca_ipv6_t":
                    set_proc += tab + "memcpy(cfg->" + f[1] + "," + f[1] + ",sizeof("+f[0]+"));\n"
                    get_proc += tab + "return cfg->" + f[1] + ";"
                elif f[0] == "ca_mac_addr_t":
                    set_proc += tab + "memcpy(&cfg->" + f[1] + "," + f[1] + ",sizeof("+f[0]+"));\n"
                    get_proc += tab + "return &cfg->" + f[1] + ";"
                elif f[0] == "ca_pkt_block_t":
                    set_proc += tab + "cfg->" + f[1] + " = " + f[1] + ";\n"
                    get_proc += tab + "return cfg->" + f[1] + ";"
                else:
                    set_proc += tab + "memcpy(&cfg->" + f[1] + "," + f[1] + ",sizeof("+f[0]+"));"
                    get_proc += tab + "return &cfg->" + f[1] + ";"
                
            get_func.params = get_params;
            set_func.params = set_params;
            get_func.defs = defs
            set_func.defs = defs
            set_func.var_defs = set_vars;
            set_func.proc = set_proc;
            get_func.proc = get_proc;

            self.funcs.append(set_func)
            self.funcs.append(get_func)

    @staticmethod
    def replace_buildin_types(string,enums):
        ret = string
        #ret = ret.replace("ca_status_t","unsigned short");
        for data_type in ca_types2_dicts.keys():
            ret = ret.replace(data_type,ca_types2_dicts[data_type]);
        for data_type in ca_types_dicts.keys():
            ret = ret.replace(data_type,ca_types_dicts[data_type]);
        for data_type in enums.keys():
            ret = ret.replace(data_type,"unsigned int")
        for data_type in ca_buildin_enums:
            ret = ret.replace(data_type,"unsigned int")
        return ret

    def get_define_code(self):
        ret = ""
        for func in self.funcs:
            ret += func.gen_define()
        ret = self.replace_buildin_types(ret,self.enums)
        return ret

    def gen_implement_code(self):
        ret = ""
        for func in self.funcs:
            ret += func.gen_define();
        for func in self.funcs:
            ret += func.gen_c_code()
        return ret

    def get_tcl_funcs(self):
        self.funcs = []
        self.gen_structs(self.structs)
        self.gen_unions(self.unions)
        return (self.gen_implement_code(),self.get_define_code())

    def toString(self):
        ret = ""
        for func in self.funcs:
            ret += func.gen_define()
        return ret

class SdkApiHeaderFile:
    def __init__(self,headerFile):
        self.headerFile = headerFile
        cHeader =  CHeader(self.headerFile)
        self.methods = cHeader.methods
        self.structs = cHeader.structs
        self.unions = cHeader.unions
        self.enums = cHeader.enums

        self.valid = True
        self.validate();

        self.msg_structs = []
        self.stub_funcs = []
        self.cb_funcs = [] 
        self.hton_funcs = [] 
        self.sdk_defs = []

    def gen_sdk_defines(self):
        methods = self.methods
        tab = "        "
        sdk_defs = ""
        for func_name in methods.keys():
            sdk_defs = self.get_msg_id(func_name)
            sdk_defs += ",\t" +  self.get_cb_func_name(func_name)  
            self.sdk_defs.append(sdk_defs);

    def get_sdk_defs(self):
        self.sdk_defs = []
        self.gen_sdk_defines();
        return self.sdk_defs

    def get_msg_structs(self):
        self.msg_structs = []
        self.gen_msg_structs()
        gen_str = ""
        for struct in self.msg_structs:
            gen_str += struct
        return gen_str

    def gen_msg_structs(self):
        methods = self.methods
        tab = "    "

        for func_name in methods.keys():
            params = methods[func_name].params;
            gen_str = ""
            for macro in methods[func_name].defs:
                if macro.first_ndef:
                    continue;
                if macro.ndef:
                    gen_str += "#ifndef " + macro.name + "\n"
                else:
                    gen_str += "#ifdef " + macro.name + "\n"
            gen_str += "typedef struct{ \n"

            remain_size = 0;
            rsvd = 1
            for param in params:
                field = ""
                rsvd_field = ""
                #if the param type is void pointer
                if param[0] == "void":
                    field += tab + param[0] + " *"  + param[2]
                else: # not void pointer
                    field += tab + param[0] + " "  + param[2] 
                if param[5] != 0:
                    field +="[" + str(param[5]) + "]"
                field += ";\n"

                size = param[5]
                if size == 0:
                    size = 1
                #mac treat as ca_uint8 [6]
                data_type = param[0]
                if data_type == "ca_mac_addr_t":
                    data_type = "ca_uint8"
                    size = 6

                if not ca_types_size_table.has_key(data_type) or ca_types_size_table[data_type] not in [1,2] :
                    if remain_size == 0:
                        gen_str += rsvd_field + field
                        continue

                    rsvd_field = tab + "ca_uint8  rsvd_f" + str(rsvd)
                    if remain_size != 1:
                        rsvd_field += "[" + str(remain_size) + "]"
                    rsvd_field += ";\n"
                    rsvd += 1

                    remain_size = 0
                    gen_str += rsvd_field + field
                    continue

                if ca_types_size_table[data_type] == 1:
                    f_remain = 4 - size%4
                    remain_size  = remain_size + f_remain
                elif ca_types_size_table[data_type] == 2:
                    f_remain = 4 - (size * 2 )%4
                    if remain_size in [0,2]:
                        remain_size += f_remain
                    elif remain_size == 3:
                        rsvd_field = tab + "ca_uint8  rsvd_" + str(rsvd) + ";\n"
                        rsvd += 1
                        remain_size = f_remain - 2;
                    else: #1
                        rsvd_field = tab + "ca_uint8  rsvd_" + str(rsvd) + ";\n"
                        rsvd += 1
                        remain_size = f_remain

                remain_size = remain_size %4
                gen_str += rsvd_field + field

            #last remain size
            if remain_size == 1:
                gen_str += tab + "ca_uint8 rsvd_f" + str(rsvd) + ";\n"
            elif remain_size > 1:
                gen_str += tab + "ca_uint8 rsvd_f" + str(rsvd) + "["+str(remain_size)+"];\n"

            gen_str +="}__attribute__((packed)) " + self.get_msg_wrap_structs_name(func_name) + ";\n"

            for macro in methods[func_name].defs:
                if macro.first_ndef:
                    continue;
                gen_str += "#endif \n"

            gen_str + "\n"
            self.msg_structs.append(gen_str);

    def get_stub_funcs(self):
        self.stub_funcs = []
        self.gen_stub_funcs()
        gen_str = ""
        for func in self.stub_funcs:
            gen_str += func.gen_c_code()
        return gen_str

    def get_stub_funcs_define(self):
        self.stub_funcs = []
        self.gen_stub_funcs()
        gen_str = ""
        for func in self.stub_funcs:
            gen_str += func.gen_define()
        return gen_str

    def gen_stub_funcs(self):
        methods = self.methods
        structs = self.structs 
        tab = "    "
        for func_name in methods.keys():
            method = methods[func_name]
            stub_func = FuncString(method.ret_type,method.name)
            stub_func.defs = method.defs
            tab = "    "
            stub_func.var_defs = tab + "ca_int8 buf[CA_SDK_MSG_MAX_LEN]={0};\n"
            stub_func.var_defs += tab + "ca_sdk_msg_t *pMsg = (ca_sdk_msg_t *)buf; \n"
            stub_func.var_defs += tab + "ca_status_t result = CA_E_ERROR; \n"
            stub_func.var_defs += tab + self.get_msg_wrap_structs_name(func_name) + " * _wrap_cfg = "
            stub_func.var_defs +=  "(" + self.get_msg_wrap_structs_name(func_name) + "*)(pMsg->data);\n"
            
            params = methods[func_name].params;
            sub_len = ""
            if params[-1][5] != 0 and not params[-1][6].startswith("*"):
                sub_len = " - (" + str(params[-1][5]) + " - " + params[-1][6] + ") * sizeof("+ params[-1][0]+")"

            errs = []
            if "iterate" in func_name:
                proc = tab + "if(4096 < (return_entry->user_buffer_size)){\n"
                proc += tab + tab + "return CA_E_PARAM;\n" 
                proc += tab + "}\n\n"
            else:
                proc = tab + "\n\n"
            proc += tab + "pMsg->len = sizeof(" + self.get_msg_wrap_structs_name(func_name) + ")" + sub_len + ";\n"
            proc += tab + "pMsg->sdk_id = " + self.get_msg_id(func_name) + ";\n"
            first = True
            ca_in =  ["CA_IN","CA_OUT","CA_IN_OUT"]
            ca_out = ["CA_OUT","CA_IN_OUT"]
            in_str = ""
            out_str = ""
            for param in params:
                if not first:
                    stub_func.params +=","
                else:
                    first = False
                #print param
                stub_func.params += "\n" + tab +  param[0] + " " + param[1] + " " + param[2]

                if param[0] != "void" and param[1] == "*":
                    errs.append("NULL == " + param[2])
                if param[0] == "ca_mac_addr_t" and param[1] != "*":
                    param[1] = " * "
                    errs.append(param[2] + "== NULL")
                
                if param[4] in ca_in:
                    if param[1] != "*":
                        if  param[0]  in ca_basic_types or self.enums.has_key(param[0]) or param[0] in ca_buildin_enums:
                            in_str += tab + "_wrap_cfg->" + param[2]  + " = " + param[2] + ";\n"
                        elif param[0] in ca_buildin_fpointers:
                            #in_str += tab + "if(" + param[2] + "){\n"
                            #in_str += tab + tab + "_wrap_cfg->" + param[2]  + " = &" + param[2] + ";\n"
                            #in_str += tab + "}\n"
                            in_str += tab + "_wrap_cfg->" + param[1]  + " = " + param[1] + ";\n"
                        elif param[0] == "ca_ipv6_t":
                            in_str += tab + "memcpy(&(_wrap_cfg->" + param[2]  + "[0]),&" + param[2] + "[0],sizeof(" + param[0] + "));\n"
                        elif param[0] == "ca_mac_addr_t":
                            in_str += tab + "memcpy((ca_uint8_t *)_wrap_cfg->" + param[2]  + "," + "(ca_uint8_t *)" + param[2] + ",sizeof(" + param[0] + "));\n"
                        elif param[0]  in ca_buildin_structs or structs.has_key(param[0]):
                            in_str += tab + "memcpy(&_wrap_cfg->" + param[2]  + ",&" + param[2] + ",sizeof(" + param[0] + "));\n"
                    else:
                        if param[5] == 0:
                            if  param[0]  in ca_basic_types or self.enums.has_key(param[0]) or param[0] in ca_buildin_enums:
                                if  param[0] == "void": # if is void *
                                    in_str += tab + "_wrap_cfg->" + param[2]  + " = " + param[2] + ";\n"
                                else: # not is void *
                                    in_str += tab + "_wrap_cfg->" + param[2]  + " = *" + param[2] + ";\n"
                            elif param[0] == "ca_ipv6_t":
                                in_str += tab + "memcpy(&(_wrap_cfg->" + param[2]  + "[0])," + param[2] + "[0],sizeof(" + param[0] + "));\n"
                            elif param[0]  in ca_buildin_structs or structs.has_key(param[0])  :
                                in_str += tab + "memcpy(&_wrap_cfg->" + param[2]  + "," + param[2] + ",sizeof(" + param[0] + "));\n"
                        else:
                            in_str += tab + "memcpy(_wrap_cfg->" + param[2] + "," + param[2] + ",sizeof(" + param[0] + ") * (" + param[6] +  "));\n"

                if param[4] in ca_out:
                    if param[1] != "*":
                        if  param[0]  in ca_basic_types or self.enums.has_key(param[0]) or param[0] in ca_buildin_enums: 
                            out_str += tab + param[2] + " = _wrap_cfg->" + param[2]  + ";\n"
                        elif param[0] == "ca_ipv6_t":
                            out_str += tab + "memcpy(&" + param[2] + "[0],&(_wrap_cfg->" + param[2]  + "[0]),sizeof(" + param[0] + "));\n"
                        elif param[0]  in ca_buildin_structs or structs.has_key(param[0])  :
                            out_str += tab + "memcpy(&" + param[2] + ",&_wrap_cfg->" + param[2]  + ",sizeof(" + param[0] + "));\n"
                    else:
                        if param[5] == 0:
                            if  param[0]  in ca_basic_types or self.enums.has_key(param[0]) or param[0] in ca_buildin_enums:
                                if param[0] == "void":
                                    out_str += tab + param[2] + " = _wrap_cfg->" + param[2]  + ";\n"
                                else:
                                    out_str += tab + "*" + param[2] + "= _wrap_cfg->" + param[2] + ";\n"
                            elif param[0] == "ca_ipv6_t":
                                out_str += tab + "memcpy(" + param[2] + "[0],&(_wrap_cfg->" + param[2]  + "[0]),sizeof(" + param[0] + "));\n"
                            elif param[0]  in ca_buildin_structs or structs.has_key(param[0])  :
                                out_str += tab + "memcpy(" + param[2] + ",&_wrap_cfg->" + param[2]  + ",sizeof(" + param[0] + "));\n"
                        else:
                            out_str += tab + "memcpy(" + param[2] + ",_wrap_cfg->" + param[2] +  ",sizeof(" + param[0] + ") * (" + param[6] +  "));\n"
            proc += in_str + "\n"
            proc += tab + "result =  ca_sdk_ioctl("+params[0][2] +",pMsg); \n\n"
            proc +=tab + "if(result != CA_E_OK && result != CA_E_EXISTS){\n"
            proc +=tab + tab + "return result; \n"
            proc +=tab + "}\n\n"
            proc += out_str + "\n";
            proc +=tab + "return result; "
            err_str = ""
            if len(errs) != 0:
                err_str = tab + "if("
                first = True
                for err in errs:
                    if not first:
                        err_str +=" || "
                    first = False

                    err_str += err;
                err_str +="){\n"
                err_str += tab + tab + "return CA_E_PARAM;\n"
                err_str += tab + "}\n\n"
            stub_func.proc  = err_str + proc

            self.stub_funcs.append(stub_func)

    def get_cb_funcs(self):
        self.cb_funcs = []
        self.gen_cb_funcs()
        imp_str = ""
        def_str = ""
        for func in self.cb_funcs:
            imp_str += func.gen_c_code()
            def_str += func.gen_define()
        return (imp_str,def_str)

    def gen_cb_funcs(self):
        methods = self.methods
        for func_name in methods.keys():
            tab ="    "
            cb_func = FuncString("ca_status_t", self.get_cb_func_name(func_name))
            cb_func.params = "ca_uint8 *data, ca_uint32 size"
            cb_func.var_defs = "" 
            for macro in methods[func_name].defs:
                if macro.first_ndef:
                    continue;
                if macro.ndef:
                    cb_func.var_defs +="#ifndef " + macro.name + "\n"
                else:
                    cb_func.var_defs +="#ifdef " + macro.name + "\n"
            #if len(methods[func_name].params) != 1:
            cb_func.var_defs += tab + self.get_msg_wrap_structs_name(func_name) + "* cfg = ("
            cb_func.var_defs += self.get_msg_wrap_structs_name(func_name) + " *) data;\n"

            if "iterate" in func_name:
                #cb_func.var_defs += tab + "/* ######### */\n"
                cb_func.var_defs += tab + "void * p = cfg->return_entry.entry_data;\n"
                cb_func.var_defs += tab + "void * tmp =  cfg->return_entry.entry_data;\n"
                cb_func.var_defs += tab + "ca_uint32_t buffer_size= cfg->return_entry.user_buffer_size;\n"
                cb_func.var_defs += tab + "ca_int32_t ret;\n"
                cb_func.proc = tab + "cfg->return_entry.entry_data = ca_malloc(buffer_size);\n"
                cb_func.proc += tab + "copy_from_user(cfg->return_entry.entry_data,p,buffer_size);\n"
                cb_func.proc += tab + "ret = " + func_name + "("
                first = True
                params = methods[func_name].params;
                for param in params:
                    if first:
                        #cb_func.proc+="0"
                        cb_func.proc +="cfg->" + param[2]
                        first = False
                        continue
                    else:
                        cb_func.proc +=","
                    if param[5] == 0 and param[1] != "*" and param[0] != "ca_mac_addr_t":
                        cb_func.proc +="cfg->" + param[2]
                    elif param[5] == 0 and param[1] == "*":
                        cb_func.proc +="&cfg->" + param[2]
                    elif param[5] != 0  and param[1] == "*":
                        cb_func.proc +="cfg->" + param[2] 
                    if param[0] == "ca_mac_addr_t":
                        cb_func.proc +="&cfg->" + param[2]
                cb_func.proc += ");\n"
                #ret =  ca_tunnel_iterate(cfg->device_id,&cfg->return_entry);
                cb_func.proc += tab + "if(ret != 0){\n"
                cb_func.proc += tab + tab + "void * tmp =  cfg->return_entry.entry_data;\n";
                cb_func.proc += tab + tab + "cfg->return_entry.entry_data = p;\n";
                cb_func.proc += tab + tab + "ca_free(tmp);\n";
                cb_func.proc += tab + tab + "return ret;\n";
                cb_func.proc += tab + "}\n\n"
                cb_func.proc += tab + "copy_to_user(p,cfg->return_entry.entry_data,buffer_size);\n"
                cb_func.proc += tab + "tmp =  cfg->return_entry.entry_data;\n"
                cb_func.proc += tab + "cfg->return_entry.entry_data = p;\n"
                cb_func.proc += tab + "ca_free(tmp);\n\n"
                cb_func.proc += tab + "return ret;\n";
            else:
                #print ""
                cb_func.proc = tab + "return " + func_name + "("
                first = True
                params = methods[func_name].params;
                for param in params:
                    if first:
                        #cb_func.proc+="0"
                        cb_func.proc +="cfg->" + param[2]
                        first = False
                        continue
                    else:
                        cb_func.proc +=","
                    if param[5] == 0 and param[1] != "*" and param[0] != "ca_mac_addr_t":
                        cb_func.proc +="cfg->" + param[2]
                    elif param[5] == 0 and param[1] == "*":
                        cb_func.proc +="&cfg->" + param[2]
                    elif param[5] != 0  and param[1] == "*":
                        cb_func.proc +="cfg->" + param[2] 
                    if param[0] == "ca_mac_addr_t":
                        cb_func.proc +="&cfg->" + param[2]
                cb_func.proc += ");\n"

            for macro in methods[func_name].defs:
                if macro.first_ndef:
                    continue;
                cb_func.proc +="#endif \n" 
            if  len(methods[func_name].defs) > 0:
                cb_func.proc +=tab + " "
                #return 0 \n;
            self.cb_funcs.append(cb_func)

    def get_data_hton_funcs(self):
        self.hton_funcs = [] 
        self.gen_data_hton_funcs()
        imp_str = ""
        def_str = ""
        for func in self.hton_funcs:
            imp_str += func.gen_c_code()
            def_str += func.gen_define()
        return (imp_str,def_str)


    @staticmethod
    def get_msg_wrap_structs_name(func_name):
        return func_name  + "_msg_wrap_t"

    @staticmethod
    def get_cb_func_name(func_name):
        return func_name  + "_callback"

    @staticmethod
    def get_msg_id(func_name):
        prefix = "ca_"
        msg_id = func_name
        if func_name.startswith(prefix):
            msg_id = func_name[len(prefix):]
        msg_id = "ca_sdk_" + msg_id
        return msg_id.upper()

    def validate(self):
        methods = self.methods
        structs = self.structs
        unions = self.unions
        verified = []
        for func_name in methods.keys():
            params = methods[func_name].params
            #print func_name
            #print params
            for param in params: 
                data_type = param[0]
                if data_type in ca_defs:
                    continue
                if data_type in verified:
                    continue
                if data_type in structs.keys():
                    self.validate_struct_and_union(data_type,structs[data_type].fields)
                    if self.valid:
                        verified.append(data_type)
                        continue
                elif data_type in self.enums.keys():
                    continue
                elif data_type in unions.keys():
                    self.validate_struct_and_union(data_type,unions[data_type].fields)
                    if self.valid:
                        verified.append(data_type)
                        continue
                else :
                    print "Error:" + data_type + " has not found define"
        if not self.valid:
            sys.exit(1)

    def validate_struct_and_union(self,data_type,fields):
        structs = self.structs
        unions = self.unions
        for field in fields:
            f_type = field[0]
            if f_type in ca_defs:
                continue
            if f_type in structs.keys():
                self.validate_struct_and_union(f_type,structs[f_type].fields)
                if self.valid:
                    continue
            elif f_type in self.enums.keys():
                continue
            elif f_type in unions.keys():# and len(field) == 5 and field[4]["union"]:
                self.validate_struct_and_union(f_type,unions[f_type].fields)
                if self.valid:
                    continue
            else :
                print "Error: In struct " + data_type + ",field " + f_type + " has not found define"

class CodeFile:
    def __init__(self,hfile,o_dir,test_dir):
        fileName = os.path.basename(hfile)
        self.hfile = hfile
        self.stub_file = o_dir + "/user/" +fileName.replace(".h","_user.c");
        self.msg_wrap_hfile = o_dir + "/inc/" + fileName.replace(".h","_msg_wrap.h");
        self.cb_cfile =  o_dir + "/driver/" + fileName.replace(".h","_handler.c");
        self.cb_hfile =  o_dir + "/driver/" + fileName.replace(".h","_handler.h");
        self.tcl_data_file = test_dir +  fileName.replace(".h","_tcl.c");
        self.tcl_i_file =    test_dir +  fileName.replace(".h","_tcl.i");
        self.tcl_h_file =    test_dir +  fileName.replace(".h","_tcl.h");
        self.SdkApiHeader = SdkApiHeaderFile(self.hfile)

    def create(self):
        self.create_structs_file(self.SdkApiHeader)
        self.create_stubs_file(self.SdkApiHeader)
        self.create_cb_file(self.SdkApiHeader)
        self.create_tcl_file(self.SdkApiHeader)

    def get_sdk_defines(self):
        return self.SdkApiHeader.get_sdk_defs();

    def create_structs_file(self,SdkApiHeader):
        #struct header file
        gen_str,end_str = self.get_header_macro(os.path.basename(self.msg_wrap_hfile))
        gen_str += "#include \"" + os.path.basename(self.hfile) + "\"\n"
        gen_str += SdkApiHeader.get_msg_structs()
        gen_str += end_str + "\n\n"
        msg_wrap_hfile = open(self.msg_wrap_hfile,"w");
        msg_wrap_hfile.write(gen_str)
        msg_wrap_hfile.flush()
        msg_wrap_hfile.close()

    def create_stubs_file(self,SdkApiHeader):
        #stub c file
        gen_str = "#include <stdio.h> \n"
        gen_str += "#include <stdlib.h> \n"
        gen_str += "#include <string.h> \n"
        gen_str += self.get_cmn_header_include()  + "\n\n"
        gen_str += SdkApiHeader.get_stub_funcs()
        stub_file = open(self.stub_file,"w");
        stub_file.write(gen_str)
        stub_file.flush()
        stub_file.close()

    def create_cb_file(self,SdkApiHeader):
        c_str = self.get_cmn_header_include() 
        c_str += "#include \"osal_mem.h\" \n"
        c_str += "#include <linux/uaccess.h> \n"
        c_str += "#include \"" + os.path.basename(self.cb_hfile) + "\"\n\n"
        #c_str += "#include \"" + os.path.basename(self.hfile) + "\"\n\n"
        h_str,end_str = self.get_header_macro(os.path.basename(self.cb_hfile))
        imp_str,def_str = SdkApiHeader.get_cb_funcs()

        c_str += imp_str + "\n\n"
        cb_cfile = open(self.cb_cfile,"w");
        cb_cfile.write(c_str)
        cb_cfile.flush()
        cb_cfile.close()

        h_str += def_str + end_str + "\n\n"

        cb_hfile = open(self.cb_hfile,"w");
        cb_hfile.write(h_str)
        cb_hfile.flush()
        cb_hfile.close()

    def create_tcl_file(self,SdkApiHeader):
        head_str = "#include <stdio.h> \n"
        head_str += "#include <stdlib.h> \n"
        head_str += "#include <string.h> \n"
        head_str += self.get_cmn_header_include() + "\n\n"
        tcl_funcs = SdkTCLFunction(SdkApiHeader)
        imp_str,i_str = tcl_funcs.get_tcl_funcs()
        c_str = head_str + imp_str
        tcl_stub = i_str
        i_str += SdkTCLFunction.replace_buildin_types(SdkApiHeader.get_stub_funcs_define(),self.SdkApiHeader.enums)

        tcl_data_file = open(self.tcl_data_file,"w");
        tcl_data_file.write(c_str)
        tcl_data_file.flush()
        tcl_data_file.close()

        tcl_i_file = open(self.tcl_i_file,"w");
        tcl_i_file.write(i_str)
        tcl_i_file.flush()
        tcl_i_file.close()

        tcl_h_file = open(self.tcl_h_file,"w");
        tcl_h_file.write(head_str + tcl_stub)
        tcl_h_file.flush()
        tcl_h_file.close()

    def get_header_macro(self,file_name):
        start_str = "#ifndef __" + file_name.replace(".","_").upper() + "__\n"
        start_str += "#define __" + file_name.replace(".","_").upper() + "__\n\n"
        end_str = "#endif"

        return (start_str,end_str)
    def get_cmn_header_include(self):
        c_str = "#include \"ca_types.h\" \n"
        c_str += "#include \"ca_sdk_msg.h\" \n"
        c_str += "#include \"ca_sdk_linux.h\" \n"
        c_str += "#include \"" + os.path.basename(self.hfile) + "\"\n"
        c_str += "#include \"" + os.path.basename(self.msg_wrap_hfile) + "\"\n"
        return c_str

def replace_code_gen_tag(filename,tag,gen_str):
    template_str =""
    with open(filename, 'r') as f:
        template_str = f.read()

    with open(filename, 'w') as f:
        replace_str= template_str.replace(tag,gen_str);
        f.write(replace_str)
        f.close()

def main():
    if len(sys.argv) < 3:
        print sys.argv[0] + "<out_dir> <demo.i> <header file1>"
        return

    o_dir = sys.argv[1]
    inc_odir =   sys.argv[1] + "/inc/"
    csapi_odir = sys.argv[1] + "/user/"
    test_odir = sys.argv[1] + "/test/" 
    msghandler_odir = sys.argv[1] + "/driver/"

    sdk_msg = "ca_sdk_info.h"
    handler = "ca_sdk_handler.h"

    demo = sys.argv[2]
    header = sys.argv[3:]

    if len(header) == 0 :
        return

    dirs = [o_dir,inc_odir,csapi_odir,test_odir,msghandler_odir]
    for odir in dirs:
        if not os.path.exists(odir) :
            os.mkdir(odir)

    sdkdefs = []
    tcl_inc = ""
    tcl_h_inc = ""
    handler_inc = "#ifndef __CA_SDK_HANDLER_H__ \n"
    handler_inc += "#define __CA_SDK_HANDLER_H__\n"
    for hfile in header:
        print("processing file" + hfile)
        codeFile = CodeFile(hfile,o_dir,test_odir)
        codeFile.create()
        sdkdefs += codeFile.get_sdk_defines()
        #gen the tcl .i file lists
        tcl_inc += "%include " +  os.path.basename(codeFile.tcl_i_file) + "\n"
        tcl_h_inc += "#include \"" +  os.path.basename(codeFile.tcl_h_file) + "\"\n"
        handler_inc += "#include \"" +  os.path.basename(codeFile.msg_wrap_hfile) + "\"\n"
        handler_inc += "#include \"" +  os.path.basename(codeFile.cb_hfile) + "\"\n"

    handler_inc += "#endif \n"

    handler_hfile = open(msghandler_odir + "/" + handler,"w");
    handler_hfile.write(handler_inc)
    handler_hfile.flush()
    handler_hfile.close()

    #replace the sdk_msg.h
    index = 3
    gen_str = "#ifndef __CA_SDK_INFO_H__ \n"
    gen_str += "#define __CA_SDK_INFO_H__ \n"
    gen_str += "#define CA_SDK_API_LIST_INFO \\\n"
    tab = "    "
    for sdkdef in sdkdefs:
        index +=1
        gen_str += tab + tab +  "xx(" + str(index) + ",\t" + sdkdef + ") \\\n"
    gen_str +="\n#endif \n"

    dest_demo = test_odir + "/test.i"
    if os.path.exists(dest_demo):
        os.remove(dest_demo)

    copyfile(demo, dest_demo)

    replace_code_gen_tag(dest_demo,"/*__CODE_GEN_TAG__*/",tcl_inc)
    replace_code_gen_tag(dest_demo,"/*__CODE_GEN_H_TAG__*/",tcl_h_inc)

    handler_hfile = open(inc_odir + "/" + sdk_msg ,"w");
    handler_hfile.write(gen_str)
    handler_hfile.flush()
    handler_hfile.close()

if __name__ == "__main__":
    main()
