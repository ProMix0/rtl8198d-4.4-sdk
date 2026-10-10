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
]

ca_module_tokens = [
    "port_",
    "qos_",
    "tm_",
    "fdb_",
    "vlan_",
    "classifier_"
]

t_ignore = " \t\r.|:!?%@"
t_NUMBER = r'[0-9][0-9XxA-Fa-f]*'
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
            cmn_str +=" "
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
        self.name = tmpList[1]
        self.value = "".join(tmpList[2:])
        if debug :
            print self.name," = ",self.value

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

            self.fields.append(field)
        if debug :
            print self.fields
        #get struct name 
        preBraceStack = nameStack[:nameStack.index("{")]
        postBraceStack = nameStack[nameStack.index("}") + 1:]
        if (len(preBraceStack) == 2 and "typedef" not in nameStack):
            self.name =  preBraceStack[1]           
        elif len(postBraceStack) and "typedef" in nameStack:
            self.name =  " ".join(postBraceStack)


class CMethod:
    def __init__(self, nameStack):
        self.name = ""
        if len(nameStack) < 4 or "(" not in nameStack or ")" not in nameStack:
            #Not enough stuff for an enum
            return
        self.params = []
        self.name = ""
        self.ret_type = ""
        decoration = [ "CA_IN", "CA_OUT", "CA_IN_OUT","const"]

        for dec in decoration:
            while (dec in nameStack ):
                nameStack.remove(dec)
       
        self.ret_type = " ".join(nameStack[:nameStack.index('(') - 1])
        self.name = " ".join(nameStack[nameStack.index('(') - 1:nameStack.index('(')])

        paramsStack = nameStack[nameStack.index('(') + 1: ]
        while (paramsStack[-1]  != ")"):
            paramsStack.pop()
        paramsStack.pop()

        #no parameters 
        if len(paramsStack) == 1 and paramsStack[0] == "void" :
            return
        
        #igore the function
        if paramsStack[0].endswith("*/"): 
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
        self.name = os.path.basename(headerFileName).replace("paal_","").replace(".h","");
        self.enums = {}
        self.structs = {}
        self.methods = {}
        self.macros = {}

        self.nameStack = []
        self.braceDepth  = 0

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
            self.nameStack = []
            return
        if( is_enum(self.nameStack) ):
            newEnum = CEnum(self.nameStack)
            if newEnum.name != "":
                self.enums[newEnum.name] = newEnum
            self.nameStack = []
            return
        elif( is_struct(self.nameStack)):
            newStruct = CStruct(self.nameStack)
            if newStruct.name != "":
                self.structs[newStruct.name] = newStruct
            self.nameStack = []
        else :
            newMethod = CMethod(self.nameStack)
            if newMethod.name != "":
                self.methods[newMethod.name] = newMethod
            self.nameStack = []

    def evaluate_macro(self, macro):
        newMacro = CMacro(macro)
        if newMacro.name != "" :
            self.macros[newMacro.name] = newMacro
        


###############################################################################
# AAL CLI code gen
###############################################################################
cli_prefix = "cli_paal_"
paal_prefix = "ca_paal_"
cfg_set_sufix = "_set"
cfg_get_sufix = "_get"
struct_sufix = "_t"

def get_func_tag(func):
    tag = ""
    if func.endswith(cfg_set_sufix) or func.endswith(cfg_get_sufix):
        tag = func[:-len(cfg_set_sufix)]
    if func.startswith(paal_prefix):
        tag = tag[len(paal_prefix):]
    return tag

def is_set_func( func_name):
    if(func_name == ""):
        return False
    if (func_name.startswith(paal_prefix) and func_name.endswith(cfg_set_sufix)) : 
        return True
    return False

def is_get_func (func_name):
    if(func_name == ""):
        return False
    if (func_name.startswith(paal_prefix) and func_name.endswith(cfg_get_sufix)) : 
        return True
    return False

def is_std_func(func_name):
    return is_get_func(func_name) or is_set_func(func_name)

def gen_set_func(tag):
    return paal_prefix + tag + cfg_set_sufix

def gen_get_func(tag):
    return paal_prefix + tag + cfg_get_sufix

def get_cli_get_handler_func(tag):
    return cli_prefix + tag  + cfg_get_sufix;

def get_cli_get_cmd(tag):
    return cli_prefix + tag  + cfg_get_sufix + "_cmd";

def get_cli_set_handler_func(tag,field):
    return cli_prefix + tag  +"_" +field + cfg_set_sufix;

def get_cli_set_cmd(tag,field):
    return cli_prefix + tag  +"_" +field + cfg_set_sufix + "_cmd";

ca_ints = [
    "ca_boolean_t",
    "ca_uint8_t",
    "ca_int8_t",
    "ca_uint16_t",
    "ca_int16_t",
    "ca_uint32_t",
    "ca_int32_t",
    "ca_uint64_t",
    "ca_int64_t",
    "ca_llid_t",
    "ca_port_id_t",
    "ca_device_id_t",
    "ca_chip_id_t",
    "ca_core_id_t",
    "ca_sub_id_t",
    "ca_bank_id_t",
    "ca_voq_id_t",
    "ca_buf_id_t",
    "ca_onu_id_t"
]

#below is cortina define fake struct, the enum is fake too
fack_structs = [
   "ca_ip_address_t",
   "ca_boolean_t",
   "ca_uint8_t",
   "ca_int8_t",
   "ca_uint16_t",
   "ca_int16_t",
   "ca_uint32_t",
   "ca_int32_t",
   "ca_uint64_t",
   "ca_int64_t",
   "ca_llid_t",
   "ca_chip_id_t",
   "ca_device_id_t",
   "ca_port_id_t",
   "ca_llid_t",
   "ca_chip_id_t",
   "ca_core_id_t",
   "ca_llid_bmp_t",
   "ca_sub_id_t",
   "ca_mac_addr_t",
   "ca_sub_id_t",
   "ca_bank_id_t",
   "ca_voq_id_t",
   "ca_buf_id_t",
   "ca_onu_id_t"
]
        
        
class AALStdFunction:
    def __init__(self):
        self.file_tag = ""
        self.tag = ""
        self.enums = {}
        self.mode_cfg = ""
        self.get_func_name = ""
        self.get_params = []
        self.set_func_name = ""
        self.set_params = []
        self.has_struct = False
        self.cfg_structs = {}
        self.error = False;

    def is_int(self,data_type):
        if data_type in ca_ints or data_type in self.enums :
            return True
        else:
            return False
    def is_struct(self,data_type):
        if not data_type.endswith(struct_sufix):
            return False

        if data_type in fack_structs or data_type in self.enums or self.is_set_mask(data_type):
            return False
        return True
    
    def is_set_mask(self,data_type):
        if data_type.endswith("_mask_t"):
            return True
        else:
            return False
    def remove_tag_and_suffix(self,name):
        tag = name
        if name.startswith(self.file_tag+"_" ):
            tag = name[len(self.file_tag+"_"):]
        if tag.endswith("_cfg") :
            tag = tag[:-len("_cfg")]
        return tag

    def dump(self):
        if debug:
            print "================================================"
            print "Tag:",self.tag
            print "Mode Cfg:",self.mode_cfg
            print "Get Func:",self.get_func_name
            print "Get params:",self.get_params
            print "Set Func:",self.set_func_name
            print "Set params:",self.set_params
            print "Cfg structs:", self.cfg_structs 
            print "================================================"
        #print self.gen_get_reg()
        #print self.gen_get_handler()
        #print self.gen_set_reg()
        #print self.gen_set_handler()
    
    def gen_set_reg(self):
        #not all func has get handler
        if self.set_func_name == "" :
            return ""

        param  = self.set_params[-1]
        gen_str = ""
        if self.is_struct(param[0]): 
            #struct
            for field in self.cfg_structs[param[0]]:
                #sub struct 
                if self.is_struct(field[0]) : 
                    for f in  self.cfg_structs[field[0]]:
                        gen_str +="    CLI_KERNEL_CMD_INSTALL(" + self.mode_cfg + ",&" + get_cli_set_cmd(self.tag,param[2]  + "_"+ field[1] + "_" + f[1]) 
                        gen_str +=");\n"
                else :
                    gen_str +="    CLI_KERNEL_CMD_INSTALL(" + self.mode_cfg + ",&" + get_cli_set_cmd(self.tag,param[2]  + "_"+ field[1])
                    gen_str +=");\n"

        else:
            #single type
            gen_str += "    CLI_KERNEL_CMD_INSTALL(" + self.mode_cfg + ",&"+ get_cli_set_cmd(self.tag,param[2].lstrip("_")) + ");\n"
        return gen_str

    def gen_set_func_handler_n_cmd(self,set_tag):
        gen_str = "\n\nCLI_KERNEL_DEFINE(" + get_cli_set_handler_func(self.tag,set_tag) + "," + get_cli_set_cmd(self.tag,set_tag) + ","
        gen_str +=" \n"
        return gen_str
    
    def gen_set_func_header(self):
        gen_str = "\n)\n"
        gen_str += "#ifdef CLI_KERNEL_SPACE\n"
        gen_str += "{\n"
        return gen_str

    def gen_set_func_footer(self):
        gen_str = "\n    return CLI_CMD_OK; \n"
        gen_str += "}\n"
        gen_str += "#endif \n"
        return gen_str
    
    def merge_set_get_params(self):
        params = []
        
        get_params = copy.deepcopy(self.get_params)
        set_params = copy.deepcopy(self.set_params)

        #remove comments for match get and set, since get and set comments is not the same
        for p in get_params:
            p[3] = ""  #remove comments

        for p in set_params:
            p[3] = ""  #remove comments
        
        num = 1
        for p in get_params[1:-1]:
            if p in set_params :
                p[3] = self.set_params[num][3]  #add comments back to cli 
                num +=1
                params.append(p)
            else:
                print "ERROR: Get func parameters not match with set func"
                sys.exit(1)

        p = get_params[-1]
        p[1] = "" # change * to ""

        #add mask if have
        if len (set_params) >= 3 and self.is_set_mask(set_params[-2][0]):
            params.append(self.set_params[-2])
        
        p2 = set_params[-1]
        p2[1] = ""
        if p2 == p :
            p[3] = self.set_params[-1][3] #add comments back to cli 
            params.append(p)

        return params

    def gen_set_func_define(self,params, sub_struct = False,sub_struct_f = ""):
        gen_str ="    ca_device_id_t  device_id =  0; \n"
        
        for p in params[:-1]:
            gen_str += "    " + p[0] + "  " + p[2] + "; \n"

        p = params[-1]
        if sub_struct :
            if len(p) == 5: #array 
                gen_str += "    " + p[0] + "  " + sub_struct_f + "_" + p[2] + "; \n"
                #gen_str += "    ca_uint32_t i = 0;\n"
                gen_str += "    ca_uint32_t _index_ = 0;\n"
            else:
                gen_str += "    " + p[0] + "  " + sub_struct_f + "_" +p[2] + "; \n"
        else:
            if len(p) == 5: #array 
                gen_str += "    " + p[0] + "  " + p[2] + "; \n"
                #gen_str += "    ca_uint32_t i = 0;\n"
                gen_str += "    ca_uint32_t _index_ = 0;\n"
            else:
                gen_str += "    " + p[0] + "  " + p[2] + "; \n"

        gen_str += "    ca_status_t ret = CA_E_OK; \n"
        return gen_str

    def gen_set_init(self):
        gen_str = "\n"
        if len(self.set_params) >= 3:
            params = self.set_params[-2:]
        else:
            params = self.set_params[-1:]

        for p in params:
            if self.is_set_mask(p[0]):
                gen_str += "\n    memset(&" + p[2] + ",0xff,sizeof(" + p[0] + "));\n"

        p = params[-1]
        gen_str += "\n    memset(&" + p[2] + ",0x00,sizeof(" + p[0] + "));\n"
        return gen_str 

    def gen_set_call_get(self):
        gen_str = "\n    ret = " + self.get_func_name + "(device_id"
        for p in self.get_params[1:-1]:
            gen_str +="," + p[2]

        gen_str +=",&" + self.get_params[-1][2]
        gen_str +="); \n"
        gen_str +="    if(ret != CA_E_OK){ \n"
        gen_str +="        cli_print(cli,\"" + self.get_func_name + " failed, return = %d\\r\\n\", ret ); \n"
        gen_str +="        return CLI_CMD_E_PARAM; \n"
        gen_str +="    }\n"

        return gen_str


    def gen_set_call_set(self):
        gen_str = "\n    ret = " + self.set_func_name + "(device_id"
        for p in self.set_params[1:-1]:
            gen_str +="," + p[2]
        
        if self.set_params[-1][1] == "*" :
            gen_str +=",&" + self.set_params[-1][2]
        else:
            gen_str +="," + self.set_params[-1][2]

        gen_str +="); \n"
        gen_str +="    if(ret != CA_E_OK){ \n"
        gen_str +="        cli_print(cli,\"" + self.set_func_name + " failed, return = %d\\r\\n\", ret ); \n"
        gen_str +="        return CLI_CMD_E_PARAM; \n"
        gen_str +="    }\n"

        return gen_str
    def gen_cmd_desc(self,params):
        num = 0 
        tab = "        "
        rm_tag = self.tag + " "
        for p in ca_module_tokens:
            if rm_tag.startswith(p):
                rm_tag = rm_tag[len(p):]
        cmd_str = tab + "\"" + rm_tag
        help_str = tab + "\"set for " + self.tag + "\\n\"\n"

        has_array = False
        all_help = ""
        gen_str = ""
        param_cmd = ""
        param_help = ""
        for p in params:
            is_param = False
            if self.is_set_mask(p[0]) or self.is_struct(p[0]) : #mask, no need input
                continue
            num +=1
            node_key = p[2]

            #for nice output
            if node_key.startswith("f_"):
                node_key = node_key[2:]  
            else:
                is_param = True

            node_key = node_key.lstrip("_")
            

            if not is_param:
                cmd_str += node_key + " " 
                help_str +=tab + "\"" 
                help_str +="Set for " + node_key 
                help_str +="\\n\"\n"

                cmd_str += param_cmd
                help_str += param_help
                param_cmd = ""
                param_help = ""             

                if len(p) == 5: #array
                    cmd_str +="<0-"+str(int(p[4]) - 1)+"> "
                    help_str +=tab + "\"" 
                    help_str +="index for " + node_key 
                    help_str +="\\n\"\n"
                
            cmd_tmp = ""
            help_tmp = ""

            if p[0] == "ca_mac_addr_t":
                cmd_tmp +="A:B:C:D:E:F "
            elif p[0] == "ca_ip_address_t":
                cmd_tmp +="A.B.C.D "
            elif p[0] == "ca_uint64_t":
                cmd_tmp +="<0-0xffffffffffffffff> "
            else:
                cmd_tmp +="<0-0xffffffff> "

            if p[3] == "" :

                help_tmp +=tab + "\"" 
                help_tmp += node_key
                help_tmp +="\\n\"\n"
            else:
                help_tmp +=tab + "\"" 
                help_tmp += p[3]
                help_tmp +="\\n\"\n"
            if is_param:
                param_cmd += cmd_tmp
                param_help += help_tmp
            else:
                cmd_str += cmd_tmp
                help_str += help_tmp
    
        cmd_str += param_cmd
        help_str += param_help
        gen_str = cmd_str  + "\",\n" + help_str
        return gen_str
    
    def gen_set_get_value(self,params,sub_struct = False,sub_struct_f = ""):
        gen_str = "\n"
        num = 0
        has_array = False
        array = []
        prefix = ""
        for p in params:
            if self.is_set_mask(p[0]) or self.is_struct(p[0]) :
                continue
            if len(p) == 5: #array
                has_array = True
                num += 1
                gen_str += "    _index_ = cli_atoi(argv[" + str(num - 1) + "]);\n"

            prefix = ""
            if p == params[-1] and sub_struct:
                prefix = sub_struct_f + "_"


            num += 1
            if p[0] == "ca_mac_addr_t":
                gen_str +="    ret = ca_str2ca_mac(argv[" + str(num - 1)+ "],&" + prefix + p[2] + ");\n"
                gen_str += "    if(ret != CA_E_OK){ \n"
                gen_str += "        cli_print(cli,\"Mac address format error, should be like 01:13:25:00:00:01\\r\\n\"); \n" 
                gen_str += "        return CLI_CMD_E_PARAM; \n"
                gen_str += "    } \n"
            elif p[0] == "ca_ip_address_t":
                gen_str +="    ret = ca_str2ca_ip(argv[" + str(num - 1)+ "],&" + prefix + p[2] + ");\n"
                gen_str += "    if(ret != CA_E_OK){ \n"
                gen_str += "        cli_print(cli,\"IP address format error, should be like 192.168.1.1\\r\\n\"); \n" 
                gen_str += "        return CLI_CMD_E_PARAM; \n"
                gen_str += "    } \n"
            elif p[0] == "ca_uint64_t":
                gen_str +="    " + prefix +  p[2] + " = cli_str2ll(argv[" + str(num - 1)+ "]);\n"
            else:
                gen_str +="    " + prefix +  p[2] + " = cli_atoi(argv[" + str(num - 1)+ "]);\n"

        return gen_str
    
    def gen_set_cfg_value(self,cfg, field,sub_struct = False,sub_struct_f = ""):
        gen_str = "\n"
        cfg_name = field[2]
        var_name = field[2]

        #for struct set value, we need convert the field back
        if field[2].startswith("f_"):
            cfg_name = field[2][2:]

        if sub_struct:
            if field[2].startswith("f_"):
                cfg_name = sub_struct_f + "." + field[2][2:]
            else:
                cfg_name = sub_struct_f + "." + field[2]
            var_name = sub_struct_f + "_" + field[2]

        if len(field) == 5: #array 
            if field[0] == "ca_mac_addr_t":
                gen_str += "    memcpy("+ cfg[2] + "." + cfg_name + "[_index_].addr, " + var_name + ".addr,sizeof(ca_mac_addr_t)) ;\n"
            elif field[0] == "ca_ip_address_t":
                gen_str += "    memcpy((char *)&"+ cfg[2] + "." + cfg_name + "[_index_].afi, (char *)&" + var_name + ".afi,sizeof(ca_ip_address_t)) ;\n"
            else:
                gen_str += "    "+ cfg[2] + "." + cfg_name + "[_index_] = " + var_name + ";\n"
        else:
            gen_str +="    memcpy(&"+ cfg[2] + "." + cfg_name + ", &" + var_name + ",sizeof(" + var_name + ")); \n"
        return gen_str

    def gen_set_handler(self):
        #not all func has get handler
        if self.set_func_name == "" :
            return ""
        gen_str = ""
        param  = self.set_params[-1]

        #simple struct
        if not self.is_struct(param[0]) : 
            gen_str = self.gen_set_func_handler_n_cmd(param[2].lstrip("_"))
            params = self.merge_set_get_params()
            gen_str += self.gen_cmd_desc(params)
            gen_str += self.gen_set_func_header()
            gen_str += self.gen_set_func_define(params)
            gen_str += self.gen_set_init()
            gen_str += self.gen_set_get_value(params)
            gen_str += self.gen_set_call_set()
            gen_str += self.gen_set_func_footer()
            return gen_str

        #complicate struct
        for field in self.cfg_structs[param[0]] :
            #simple filed 
            if not self.is_struct(field[0]) : 
                gen_str += self.gen_set_func_handler_n_cmd(param[2]  + "_"+ field[1])
                params = self.merge_set_get_params()
                if field[2] != 0 :
                    params.append([field[0],"","f_"+field[1],field[3],field[2]])
                else:
                    params.append([field[0],"","f_"+field[1],field[3]])
                gen_str += self.gen_cmd_desc(params)
                gen_str += self.gen_set_func_header()
                gen_str += self.gen_set_func_define(params)
                gen_str += self.gen_set_init()
                gen_str += self.gen_set_get_value(params)
                gen_str += self.gen_set_call_get()
                gen_str += self.gen_set_cfg_value(param,params[-1])
                gen_str += self.gen_set_call_set()
                gen_str += self.gen_set_func_footer()
            else:
                for f in  self.cfg_structs[field[0]]:
                    gen_str += self.gen_set_func_handler_n_cmd(param[2]  + "_"+ field[1] + "_" + f[1])
                    params = self.merge_set_get_params()
                    cmd_des_params = self.merge_set_get_params()
                    if f[2] != 0 :
                        cmd_des_params.append([f[0],"","f_"+field[1]+f[1],f[3],f[2]])
                        params.append([f[0],"","f_"+f[1],f[3],f[2]])
                    else:
                        cmd_des_params.append([f[0],"","f_"+field[1]+f[1],f[3]])
                        params.append([f[0],"","f_"+f[1],f[3]])
                    gen_str += self.gen_cmd_desc(cmd_des_params)
                    gen_str += self.gen_set_func_header()
                    gen_str += self.gen_set_func_define(params,True,field[1])
                    gen_str += self.gen_set_init()
                    gen_str += self.gen_set_get_value(params,True,field[1])
                    gen_str += self.gen_set_call_get()
                    gen_str += self.gen_set_cfg_value(param,params[-1],True,field[1])
                    gen_str += self.gen_set_call_set()
                    gen_str += self.gen_set_func_footer()

        return gen_str


    def gen_get_reg(self):
        gen_str = "    CLI_KERNEL_GLOBAL_CMD_INSTALL(" + "&" + get_cli_get_cmd(self.tag) + ");\n"
        return gen_str

    def gen_struct_show(self,param):
        gen_str = ""
        for field in self.cfg_structs[param[0]] :
            if field[2] == 0 :
                if field[0] == "ca_mac_addr_t":
                    gen_str += "    cli_print(cli,\"%-30s:  %02x:%02x:%02x:%02x:%02x:%02x\\r\\n\",\"" +field[1] + "\""
                    gen_str += "," + param[2] + "." + field[1] + ".addr[0]"
                    gen_str += "," + param[2] + "." + field[1] + ".addr[1]"
                    gen_str += "," + param[2] + "." + field[1] + ".addr[2]"
                    gen_str += "," + param[2] + "." + field[1] + ".addr[3]"
                    gen_str += "," + param[2] + "." + field[1] + ".addr[4]"
                    gen_str += "," + param[2] + "." + field[1] + ".addr[5]"
                    gen_str += ");\n"
                elif field[0] == "ca_ip_address_t":
                    gen_str += "    cli_print(cli,\"%-30s:  afi:%d IPV4/6: %08x:%08x:%08x:%08x Addr_len:%d\\r\\n\",\"" +field[1] + "\""
                    gen_str += "," + param[2] + "." + field[1] + ".afi"
                    gen_str += "," + param[2] + "." + field[1] + ".u.ipv6_addr[0]"
                    gen_str += "," + param[2] + "." + field[1] + ".u.ipv6_addr[1]"
                    gen_str += "," + param[2] + "." + field[1] + ".u.ipv6_addr[2]"
                    gen_str += "," + param[2] + "." + field[1] + ".u.ipv6_addr[3]"
                    gen_str += "," + param[2] + "." + field[1] + ".addr_len"
                    gen_str += ");\n"
                elif self.is_struct(field[0]):
                    #sub gen_structu
                    for f in self.cfg_structs[field[0]]:
                        if f[2] == 0:
                            if f[0] == "ca_mac_addr_t":
                                gen_str += "    cli_print(cli,\"%-30s:  %02x:%02x:%02x:%02x:%02x:%02x\\r\\n\",\"" +param[2] + "." + field[1] + "." + f[1] + "\""
                                gen_str += "," + param[2] + "." + field[1] + "." + f[1] + ".addr[0]"
                                gen_str += "," + param[2] + "." + field[1] + "." + f[1] + ".addr[1]"
                                gen_str += "," + param[2] + "." + field[1] + "." + f[1] + ".addr[2]"
                                gen_str += "," + param[2] + "." + field[1] + "." + f[1] + ".addr[3]"
                                gen_str += "," + param[2] + "." + field[1] + "." + f[1] + ".addr[4]"
                                gen_str += "," + param[2] + "." + field[1] + "." + f[1] + ".addr[5]"
                                gen_str += ");\n"
                            elif f[0] == "ca_ip_address_t":
                                gen_str += "    cli_print(cli,\"%-30s:  afi:%d IPV4/6: %08x:%08x:%08x:%08x Addr_len:%d\\r\\n\",\"" +param[2] + "." + field[1] + "." + f[1] + "\""
                                gen_str += "," + param[2] + "." + field[1] + "." + f[1] + ".afi"           
                                gen_str += "," + param[2] + "." + field[1] + "." + f[1] + ".u.ipv6_addr[0]"
                                gen_str += "," + param[2] + "." + field[1] + "." + f[1] + ".u.ipv6_addr[1]"
                                gen_str += "," + param[2] + "." + field[1] + "." + f[1] + ".u.ipv6_addr[2]"
                                gen_str += "," + param[2] + "." + field[1] + "." + f[1] + ".u.ipv6_addr[3]"
                                gen_str += "," + param[2] + "." + field[1] + "." + f[1] + ".addr_len"      
                                gen_str += ");\n"             
                            elif self.is_int(f[0]):
                                gen_str += "    cli_print(cli,\"%-30s:  %u \\r\\n\"," + "\"" +  param[2] + "." + field[1] + "." + f[1] + "\"," + param[2] + "." + field[1] + "." + f[1] + ");\n" 
                            else: #u64
                                gen_str += "    cli_print(cli,\"%-30s:  %llu \\r\\n\"," + "\"" +  param[2] + "." + field[1] + "." + f[1] + "\"," + param[2] + "." + field[1] + "." + f[1] + ");\n" 
                        else:
                            for x in range(0,int(f[2])):
                                if f[0] == "ca_mac_addr_t":
                                    gen_str += "    cli_print(cli,\"%-30s:  %02x:%02x:%02x:%02x:%02x:%02x\\r\\n\",\"" +param[2] + "." + field[1] + "." + f[1]  + "["+ str(x)+"]"+ "\""
                                    gen_str += "," + param[2] + "." + field[1] + "." + f[1] +"["+str(x)+"]" + ".addr[0]"
                                    gen_str += "," + param[2] + "." + field[1] + "." + f[1] +"["+str(x)+"]" + ".addr[1]"
                                    gen_str += "," + param[2] + "." + field[1] + "." + f[1] +"["+str(x)+"]" + ".addr[2]"
                                    gen_str += "," + param[2] + "." + field[1] + "." + f[1] +"["+str(x)+"]" + ".addr[3]"
                                    gen_str += "," + param[2] + "." + field[1] + "." + f[1] +"["+str(x)+"]" + ".addr[4]"
                                    gen_str += "," + param[2] + "." + field[1] + "." + f[1] +"["+str(x)+"]" + ".addr[5]"
                                    gen_str += ");\n"
                                elif f[0] == "ca_ip_address_t":
                                    gen_str += "    cli_print(cli,\"%-30s:  afi:%d IPV4/6: %08x:%08x:%08x:%08x Addr_len:%d\\r\\n\",\"" +param[2] + "." + field[1] + "." + f[1]  + "["+ str(x)+"]"+ "\""
                                    gen_str += "," + param[2] + "." + field[1] + "." + f[1] +"["+str(x)+"]" + ".afi"           
                                    gen_str += "," + param[2] + "." + field[1] + "." + f[1] +"["+str(x)+"]" + ".u.ipv6_addr[0]"
                                    gen_str += "," + param[2] + "." + field[1] + "." + f[1] +"["+str(x)+"]" + ".u.ipv6_addr[1]"
                                    gen_str += "," + param[2] + "." + field[1] + "." + f[1] +"["+str(x)+"]" + ".u.ipv6_addr[2]"
                                    gen_str += "," + param[2] + "." + field[1] + "." + f[1] +"["+str(x)+"]" + ".u.ipv6_addr[3]"
                                    gen_str += "," + param[2] + "." + field[1] + "." + f[1] +"["+str(x)+"]" + ".addr_len"      
                                    gen_str += ");\n"                                   
                                elif self.is_int(f[0]):
                                    gen_str += "    cli_print(cli,\"%-30s:  %u \\r\\n\"," + "\"" +  param[2] + "." + field[1] + "." + f[1]  + "["+ str(x)+"]" + "\"," + param[2] + "." + field[1] + "." + f[1]  + "["+ str(x)+"]" + ");\n" 
                                else: #u64
                                    gen_str += "    cli_print(cli,\"%-30s:  %llu \\r\\n\"," + "\"" +  param[2] + "." + field[1] + "." + f[1]  + "["+ str(x)+"]" + "\"," + param[2] + "." + field[1] + "." + f[1]  + "["+ str(x)+"]" + ");\n" 

                elif self.is_int(field[0]):
                    gen_str += "    cli_print(cli,\"%-30s:  %u \\r\\n\"," + "\"" +  field[1] + "\"," + param[2] + "." + field[1] + ");\n" 
                else: # u64
                    gen_str += "    cli_print(cli,\"%-30s:  %llu \\r\\n\"," + "\"" +  field[1] + "\"," + param[2] + "." + field[1] + ");\n" 
            else:
                for x in range(0,int(field[2])): 
                    if field[0] == "ca_mac_addr_t":
                        gen_str += "    cli_print(cli,\"%-30s:  %02x:%02x:%02x:%02x:%02x:%02x\\r\\n\",\"" +field[1] + "[" + str(x) + "]"+ "\""
                        gen_str += "," + param[2] + "." + field[1] + "[" + str(x) + "]" + ".addr[0]"
                        gen_str += "," + param[2] + "." + field[1] + "[" + str(x) + "]" + ".addr[1]"
                        gen_str += "," + param[2] + "." + field[1] + "[" + str(x) + "]" + ".addr[2]"
                        gen_str += "," + param[2] + "." + field[1] + "[" + str(x) + "]" + ".addr[3]"
                        gen_str += "," + param[2] + "." + field[1] + "[" + str(x) + "]" + ".addr[4]"
                        gen_str += "," + param[2] + "." + field[1] + "[" + str(x) + "]" + ".addr[5]"
                        gen_str += ");\n"
                    elif field[0] == "ca_ip_address_t":
                        gen_str += "    cli_print(cli,\"%-30s:  afi:%d IPV4/6: %08x:%08x:%08x:%08x Addr_len:%d\\r\\n\",\"" +field[1] + "[" + str(x) + "]"+ "\""
                        gen_str += "," + param[2] + "." + field[1] + "[" + str(x) + "]" + ".afi"
                        gen_str += "," + param[2] + "." + field[1] + "[" + str(x) + "]" + ".u.ipv6_addr[0]"
                        gen_str += "," + param[2] + "." + field[1] + "[" + str(x) + "]" + ".u.ipv6_addr[1]"
                        gen_str += "," + param[2] + "." + field[1] + "[" + str(x) + "]" + ".u.ipv6_addr[2]"
                        gen_str += "," + param[2] + "." + field[1] + "[" + str(x) + "]" + ".u.ipv6_addr[3]"
                        gen_str += "," + param[2] + "." + field[1] + "[" + str(x) + "]" + ".addr_len"
                        gen_str += ");\n"
                    elif self.is_int(field[0]) :
                        gen_str += "    cli_print(cli,\"%-30s:  %u \\r\\n\"," + "\"" +field[1] + "[" + str(x) + "]" + "\"," + param[2] + "." + field[1]+ "[" + str(x) + "]"  + ");\n" 
                    else: # u64
                        gen_str += "    cli_print(cli,\"%-30s:  %llu \\r\\n\"," + "\"" +  field[1]+ "[" + str(x) + "]"  + "\"," + param[2] + "." + field[1] + "[" + str(x) + "]"  + ");\n" 
        return gen_str

    def gen_get_handler(self):
        gen_str = "\n\nCLI_KERNEL_DEFINE(" + get_cli_get_handler_func(self.tag)  + "," +  get_cli_get_cmd(self.tag) + ","
        tab = "        "
        rm_tag = ""
        for p in ca_module_tokens:
            if self.tag.startswith(p):
                rm_tag = self.tag.lstrip(p)
        cmd_str =tab + "\"show paal" 
        for p in ca_module_tokens:
            if self.tag.startswith(p):
                cmd_str += " " + p[:-1] + " "
                break;
        cmd_str += rm_tag + " "
        help_str =tab + "\"show information \\n\"\n" 
        help_str +=tab + "\"show paal" + " info \\n\"\n" 
        for p in ca_module_tokens:
            if self.tag.startswith(p):
                help_str +=tab + "\"show " + p[:-1] + " info \\n\"\n"
                break;
        help_str +=tab + "\"show " + rm_tag +" info \\n\"\n" 
        define_str ="    ca_device_id_t  device_id =  0; \n"
        define_str +="    ca_status_t ret = CA_E_OK;\n"
        init_str = ""
        func_call = "    ret = " + self.get_func_name + "(device_id"
        get_value_str = ""
        dump_str = "" 
        param_num = 0 
        all_help = ""
        for param in self.get_params[1:]: #skip device_id and core_id
            param_num += 1

            #all param start with * will treat with get parameters
            if param[1] == "*" :
                define_str +="    " + param[0] + "  " + param[2] + " ; \n"
                init_str +="    memset(&" + param[2] + ",0x00,sizeof(" + param[2] + "));\n"
                func_call +=",&" + param[2];
                if self.is_struct(param[0]): #struct
                    #dump the structure
                    dump_str = self.gen_struct_show(param)
                else:
                    if param[0] == "ca_mac_addr_t":
                        dump_str += "    cli_print(cli,\"%-30s:  %02x:%02x:%02x:%02x:%02x:%02x\\r\\n\",\"" +param[2] + "\""
                        dump_str += "," + param[2] + ".addr[0]"
                        dump_str += "," + param[2] + ".addr[1]"
                        dump_str += "," + param[2] + ".addr[2]"
                        dump_str += "," + param[2] + ".addr[3]"
                        dump_str += "," + param[2] + ".addr[4]"
                        dump_str += "," + param[2] + ".addr[5]"
                        dump_str += ");\n"
                    elif param[0] == "ca_ip_address_t":
                        dump_str += "    cli_print(cli,\"%-30s:  afi:%d IPV4/6: %08x:%08x:%08x:%08x Addr_len:%d\\r\\n\",\"" +param[2] + "\""
                        dump_str += "," + param[2] + ".afi"           
                        dump_str += "," + param[2] + ".u.ipv6_addr[0]"
                        dump_str += "," + param[2] + ".u.ipv6_addr[1]"
                        dump_str += "," + param[2] + ".u.ipv6_addr[2]"
                        dump_str += "," + param[2] + ".u.ipv6_addr[3]"
                        dump_str += "," + param[2] + ".addr_len"      
                        dump_str += ");\n"
                    elif self.is_int(param[0]):
                        dump_str += "    cli_print(cli,\"%-30s:  %u \\r\\n\"," + "\"" +  param[2].lstrip("_") + "\"," + param[2]  + ");\n" 
                    else:
                        dump_str += "    cli_print(cli,\"%-30s:  %llu \\r\\n\"," + "\"" +  param[2].lstrip("_") + "\"," + param[2]  + ");\n" 
            else: #this value need get from use input
                #setup help string
                
                if param[3] == "" :
                    help_str += tab + "\""+ param[2].lstrip("_") +"\\n\"\n"
                else:
                    help_str += tab + "\"" + param[3] + "\\n\"\n"
                #setup define 
                define_str += "    " + param[0] + " " + param[2] + ";\n";
                #setup call api 
                func_call +="," + param[2]
                #setup get value string
                if self.is_int(param[0]): 
                    cmd_str +=   "<0-0xffffffff> "
                    #dump_str += "    cli_print(cli,\"%-30s:  %u\"," + "\"" + param[0] + "\", " + param[2] + ");\n"
                    get_value_str +="    " + param[2]  + " = cli_atoi(argv[" + str(param_num - 1) + "]); \n"
                elif param[0] == "ca_mac_addr_t":
                    cmd_str +=   "A:B:C:D:E:F "
                    #dump_str += "    cli_print(cli,\"%-30s:  %02x:%02x:%02x:%02x:%02x:%02x\",\"" +param[2] + "\""
                    #dump_str += "," + param[2] + ".addr[0]"
                    #dump_str += "," + param[2] + ".addr[1]"
                    #dump_str += "," + param[2] + ".addr[2]"
                    #dump_str += "," + param[2] + ".addr[3]"
                    #dump_str += "," + param[2] + ".addr[4]"
                    #dump_str += "," + param[2] + ".addr[5]"
                    #dump_str += ");\n"
                    get_value_str += "    ret = ca_str2ca_mac(argv[" + str(param_num - 1) + "],&" + param[2] + ");\n"
                    get_value_str += "    if(ret != CA_E_OK){ \n"
                    get_value_str += "        cli_print(cli,\"Mac address format error, should be like 01:13:25:00:00:01\\n\"); \n" 
                    get_value_str += "        return CLI_CMD_E_PARAM; \n"
                    get_value_str += "    } \n"
                elif param[0] == "ca_ip_address_t":
                    cmd_str +=   "A.B.C.D "
                    get_value_str += "    ret = ca_str2ca_ip(argv[" + str(param_num - 1) + "],&" + param[2] + ");\n"
                    get_value_str += "    if(ret != CA_E_OK){ \n"
                    get_value_str += "        cli_print(cli,\"IP address format error, should be like 192.168.1.1\\n\"); \n" 
                    get_value_str += "        return CLI_CMD_E_PARAM; \n"
                    get_value_str += "    } \n"
                elif param[0] == "ca_uint64_t":
                    cmd_str +=   "<0-0xffffffffffffffff> "
                    get_value_str +="    " + param[2]  + " = cli_str2ll(argv[" + str(param_num - 1) + "]); \n"
                else:
                    print "Error parameters", param
                    sys.exit(1)

        func_call +=");\n";
        func_call +="    if(ret != CA_E_OK){ \n"
        func_call +="        cli_print(cli,\"" + gen_get_func(self.tag) + " failed, return = %d\\n\", ret ); \n"
        func_call +="        return CLI_CMD_E_PARAM; \n"
        func_call +="    }\n"
        gen_str += "\n" + cmd_str + "\",";
        gen_str += "\n" + help_str + ")\n";
        gen_str +="#ifdef CLI_KERNEL_SPACE \n"
        gen_str +="{"
        gen_str += "\n" + define_str;
        gen_str += "\n" + init_str;
        gen_str += "\n" + get_value_str;
        gen_str += "\n" + func_call
        gen_str += "\n" + dump_str
        gen_str += "\n\n" 
        gen_str +="    return CLI_CMD_OK ; \n"
        gen_str +="}\n"
        gen_str +="#endif \n"
        return gen_str


cli_config_base_id = 2001

class HeaderFile:
    def __init__(self,cmnFile, headerFile):
        file = CHeader(headerFile)
        macros = dict(CHeader(cmnFile).macros.items() +  file.macros.items()) ;
        self.enums = dict(CHeader(cmnFile).enums.items() +  file.enums.items()) ;
        self.structs = dict(CHeader(cmnFile).structs.items() +  file.structs.items()) ;
        for p in ca_module_tokens:
            if file.name.startswith(p[:-1]):
                self.mode_cfg = "CLI_NODE_ID_PAAL_" + p[:-1].upper()
                break
        else:
            self.mode_cfg = "CLI_NODE_ID_PAAL"
        self.tag = file.name 
        self.node_tag = "paal"
        self.header_file_name = os.path.basename(headerFile)
        self.expand_macro(macros)
        self.replace_macro(macros,self.structs)
        self.func_lists = []
        if debug:
            self.dump_structs(self.structs)
        self.parser_func(file.methods,self.structs)

    def expand_macro(self,cmnMacro):
        macro_oper = ["+","-","*","/"]
        macro_char = ["(",")"]
        for macro in cmnMacro :
            #remove comments at the end of the line 
            cmnMacro[macro].value = re.sub(t_COMMENT_MULTILINE,"",cmnMacro[macro].value)
            if not cmnMacro[macro].value.isdigit() :
                #convert hex to in 
                if cmnMacro[macro].value.startswith("0x"):
                    cmnMacro[macro].value = str(int(cmnMacro[macro].value,16));
                    continue
                #remove ()
                cmnMacro[macro].value = cmnMacro[macro].value.replace("(","").replace(")","");
                has_oper = False
                for oper in macro_oper:
                    if oper in cmnMacro[macro].value:
                        has_oper = True
                        tmpvalues = cmnMacro[macro].value
                        values = re.split('[\-\+\*\/]',tmpvalues)
                        ints = []
                        for value in values :              
                            if ( not value.isdigit() and not value.startswith("0x") ) and value not in cmnMacro:
                                print "Error: Macro " + macro + " define error"
                                sys.exit(1)

                            if value.startswith("0x"):
                                value = str(int(value,16))

                            if not value.isdigit():
                                value = cmnMacro[value].value
                            ints.append(int(value))
                        if oper == "+" : 
                            cmnMacro[macro].value = str(ints[0] + ints[1])
                        elif oper == "-":
                            cmnMacro[macro].value = str(ints[0] - ints[1])
                        elif oper == "*":
                            cmnMacro[macro].value = str(ints[0] * ints[1])
                        elif oper == "/":
                            cmnMacro[macro].value = str(ints[0] / ints[1])

                if not has_oper :
                    if cmnMacro[macro].value in cmnMacro :
                        cmnMacro[macro].value = cmnMacro[cmnMacro[macro].value].value
                    else:
                        print "Error: Macro " + cmnMacro[macro].value + "has not found define" 
                        sys.exit(1);

    def replace_macro(self,cmnMacro,structs):
        for struct in structs :
            for field in structs[struct].fields :
                if field[2] != 0 and not field[2].isdigit():
                    if field[2] in cmnMacro :
                        field[2] =  cmnMacro[field[2]].value

    def dump_structs(self,structs):
        for struct in structs :
            print struct,":"
            for field in structs[struct].fields :
                print "\t",field

    def is_struct(self,data_type):
        if not data_type.endswith(struct_sufix):
            return False

        if data_type in fack_structs or data_type in self.enums or data_type.endswith("_mask_t"):
            return False
        return True
    def get_cli_reg_func_name(self):
        prefix = cli_prefix + self.tag
        return prefix  + "_cmd_reg"

    def gen_cli_file(self,output_dir):
        prefix = cli_prefix + self.tag
        global cli_config_base_id
        cli_cmd_file = open(output_dir + "/" + "cli_cmd_paal_"+ self.tag + ".c", 'w')
        gen_str = "#include \"ca_types.h\" \n"
        gen_str = "#include \"cli_cmn.h\" \n"
        gen_str += "#include \"cli_product.h\" \n"
        gen_str += "#ifdef CLI_KERNEL_SPACE \n"
        gen_str += "#include \"" + self.header_file_name + "\"\n\n"
        gen_str += "#endif \n"
        cli_config_base_id = cli_config_base_id +  1
        cli_cmd_file.write(gen_str);
        gen_str = "void " + self.get_cli_reg_func_name()  + "(void) \n"
        gen_str +="{\n"
        
        get_reg_str =""
        for func in self.func_lists:
            cli_cmd_file.write(func.gen_get_handler())
            cli_cmd_file.write(func.gen_set_handler())
            get_reg_str +=  func.gen_get_reg()
            gen_str +=  func.gen_set_reg()

        cli_cmd_file.write(gen_str);
        cli_cmd_file.write(get_reg_str);
        gen_str ="}\n"
        cli_cmd_file.write(gen_str);

        cli_cmd_file.flush()
        cli_cmd_file.close()

    def parser_func(self,methods,structs):
        funcs =  methods
        while len (funcs) > 0:
            func = funcs.keys()[0]
            if(not is_std_func(func)) :  #currently, we will not process std function
                del funcs[func]
                continue

            stdFunc = AALStdFunction()

            stdFunc.tag = get_func_tag(func)
            stdFunc.file_tag = self.tag
            stdFunc.mode_cfg = self.mode_cfg
            stdFunc.enums = self.enums
            
            if is_set_func(func):
                stdFunc.set_func_name = func
                stdFunc.set_params = funcs[func].params
                del funcs[func]
                get_func = gen_get_func(stdFunc.tag)
                if(funcs.has_key(get_func)):
                    stdFunc.get_func_name = get_func
                    stdFunc.get_params = funcs[get_func].params
                    del funcs[get_func]
                else:
                    print "ERROR: function:",func," has not found get function"
                    sys.exit(1)
            else:
                stdFunc.get_func_name = func
                stdFunc.get_params = funcs[func].params
                del funcs[func]
                set_func = gen_set_func(stdFunc.tag)
                if(funcs.has_key(set_func)) : 
                    stdFunc.set_func_name = set_func
                    stdFunc.set_params = funcs[set_func].params
                    del funcs[set_func]

            if debug:
                stdFunc.dump()
            
            #check the structure define has already defined
            for param in stdFunc.set_params :
                if self.is_struct(param[0]):
                    if structs.has_key(param[0]) :
                        if not stdFunc.cfg_structs.has_key(structs[param[0]].name):
                            stdFunc.cfg_structs[structs[param[0]].name] = structs[param[0]].fields
                    else:
                        print "ERROR: struct ", param[0],"has not found define"
                        sys.exit(1)
                else: #to avoid the struct field the same as param
                    param[2] = "_" + param[2]

            for param in stdFunc.get_params :
                if self.is_struct(param[0]):
                    if structs.has_key(param[0]) :
                        if not stdFunc.cfg_structs.has_key(structs[param[0]].name):
                            stdFunc.cfg_structs[structs[param[0]].name] = structs[param[0]].fields
                    else:
                        print "ERROR: struct ", param[0],"has not found define"
                        sys.exit(1)
                else:#to avoid the struct field the same as param
                    param[2] = "_" + param[2]
            

            #struct has struture
            #for simple, we only support struct has simple struct(not include structure)
            sub_structs = {}
            for struct in stdFunc.cfg_structs:
                for field in stdFunc.cfg_structs[struct]:
                    if self.is_struct(field[0]):
                        if structs.has_key(field[0]) :
                            if not stdFunc.cfg_structs.has_key(field[0]):
                                sub_structs[structs[field[0]].name] = structs[field[0]].fields
                        else:
                            print "ERROR:struct ", field[0],"has not found define"
                            sys.exit(1)
            
            stdFunc.cfg_structs = dict( stdFunc.cfg_structs.items() + sub_structs.items() )
            stdFunc.dump()
            self.func_lists.append(stdFunc)

def main():
    if len (sys.argv) < 4:
        print sys.argv[0] + " <output dir> <reg_file>  <node header file >  <comm header file > <header file1>   <header file2>  ....."
        return
    outputdir = sys.argv[1]
    node_header = sys.argv[2]
    handler_reg_file = sys.argv[3]
    cmn_header = sys.argv[4]
    if not os.path.exists(outputdir) :
        os.mkdir(outputdir)
    
    for hfile in sys.argv[2:] :
        if not os.path.isfile(hfile):
            print hfile + " does not exist" 
            sys.exit(1)

    cli_str = "".join(open(handler_reg_file).readlines())
    cli_file = open(handler_reg_file , 'w')

    cli_node_str = "".join(open(node_header).readlines())
    cli_node_file = open(node_header, 'w')
    
    reg_str = "    "
    delcare_str = ""
    for nfile in sys.argv[5:]:
        print "Starting generate file " + nfile
        nFile = HeaderFile(cmn_header,nfile)
        nFile.gen_cli_file(outputdir)
        delcare_str += "extern void " + nFile.get_cli_reg_func_name() + "(void);\n" 
        reg_str += nFile.get_cli_reg_func_name() + "();\n    "
        print "Generate file " + nfile  + " completed"

    node_id = ""   #node file /*__PAAL_CLI_CODE_GEN__*/
    node_func_str = ""   #cli_str /*__PAAL_CLI_NODE_DEFINE_CODE_GEN__*/
    node_install_str = "" # cli_str /*__PAAL_CLI_NODE_INSTALL_CODE_GEN__*/
    for m in ca_module_tokens:
        temp_id          = "CLI_NODE_ID_PAAL_" + m[:-1].upper()
        node_id         += "    " + temp_id + ",\n"
        node_func_str     += "CLI_DEFINE(cli_node_enter_paal_" + m[:-1] + ",cli_cmd_enter_paal_" + m[:-1] + ",\"" + m[:-1]+"\",\n"
        node_func_str     += "            \"Enter " + m[:-1] + " Node \\n\")\n"
        node_func_str     += "{\n"
        node_func_str     += "    cli_enter_node(cli," + temp_id +",\"-" + m[:-1] + "\");\n";
        node_func_str     += "    return CLI_CMD_OK;\n"
        node_func_str     += "}\n"
        node_install_str +="    cli_install_cmd(CLI_NODE_ID_PAAL,&cli_cmd_enter_paal_" + m[:-1] + ");\n"
        
    cli_str = cli_str.replace("/*__PAAL_CLI_CODE_GEN__*/",reg_str);
    cli_str = cli_str.replace("/*__PAAL_CLI_DECLARE_CODE_GEN__*/",delcare_str);
    cli_str = cli_str.replace("/*__PAAL_CLI_NODE_DEFINE_CODE_GEN__*/",node_func_str);
    cli_str = cli_str.replace("/*__PAAL_CLI_NODE_INSTALL_CODE_GEN__*/",node_install_str);
    cli_node_str = cli_node_str.replace("/*__PAAL_CLI_CODE_GEN__*/",node_id);
    cli_file.write(cli_str)
    cli_file.flush()
    cli_file.close()
    
    cli_node_file.write(cli_node_str)
    cli_node_file.flush()
    cli_node_file.close()
if __name__ == "__main__":
    sys.setrecursionlimit(15000)
    main()
