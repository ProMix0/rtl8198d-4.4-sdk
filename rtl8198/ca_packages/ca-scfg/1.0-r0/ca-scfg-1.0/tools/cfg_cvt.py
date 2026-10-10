#!/usr/bin/env python3

import os
import sys
import getopt
import errno
import re
from struct import *
from operator import itemgetter

support_types = ["INT-ARRAY", "CHAR-ARRAY", "STRING",  "MAC", "INT",  "CHAR"]

special_cases = {
    "CFG_ID_MODEL_NAME": 16,
    "CFG_ID_ETH_INTF0_NAME": 16,
    "CFG_ID_ETH_INTF1_NAME": 16,
    "CFG_ID_ETH_INTF2_NAME": 16,
    "CFG_ID_ETH_INTF3_NAME": 16,
    "CFG_ID_ETH_INTF4_NAME": 16,
    "CFG_ID_ETH_INTF5_NAME": 16,
    "CFG_ID_ETH_INTF6_NAME": 16,
    "CFG_ID_ETH_INTF7_NAME": 16,
    "CFG_ID_PON_VENDOR_ID": 4,
    "CFG_ID_PON_OLT_TYPE": 8,
    "CFG_ID_ISP_TYPE": 16,
}


def cfg_value_size(cfg_type, cfg_name):
    if cfg_type == "CHAR":
        return "sizeof(uint8_t)"

    if cfg_type == "INT":
        return "sizeof(uint32_t)"

    if cfg_type == "MAC":
        return "sizeof(ca_mac_addr_t)"

    if cfg_type == "STRING" or cfg_type == "CHAR-ARRAY":
        return "sizeof(uint8_t)*%s_LEN" % cfg_name

    if cfg_type == "INT-ARRAY":
        return "sizeof(uint32_t)*%s_LEN" % cfg_name

    return


def dump_bigarray_file(cfg_array):

    parser_dict = {
        "CHAR": "parse_char",
        "CHAR-ARRAY": "parse_char_array",
        "INT":  "parse_int",
        "INT-ARRAY":  "parse_int_array",
        "MAC": "scfg_parse_mac",
        "STRING": "parse_string"
    }

    print("static struct scfg_cfg_item ca_cfg_id[CA_CFG_ID_MAX]= {")
    for cfg_item in cfg_array:
        (cfg_type, cfg_name, cfg_value) = cfg_item
        parser = parser_dict[cfg_type]
        size = cfg_value_size(cfg_type, cfg_name)
        print("{\"%s\", NULL, %-20s, %s}," % (cfg_name[7:], parser, size))
    print("};")

brace_regex = r"^{|}$"
string_quote_regex = r"^\"|\"$"

delimeters = {
    "INT-ARRAY": ',',
    "CHAR-ARRAY": ',',
    "MAC": ':',
    "CHAR": ",",
    "INT": ",",
}


def dump_def_file(cfg_array):
    for i, cfg_item in enumerate(cfg_array):
        (cfg_type, cfg_name, cfg_value) = cfg_item
        print("#define %s \"%d\"" % (cfg_name, i))
        print("#define CA_%s (%d)" % (cfg_name, i))

        if cfg_type == "STRING":
            print("#define %s_LEN (%d)" % (cfg_name, special_cases[cfg_name]))
        elif cfg_type == "INT-ARRAY" or cfg_type == "CHAR-ARRAY":
            a = re.sub(brace_regex, "", cfg_value)
            vs = a.split(delimeters[cfg_type])
            print("#define %s_LEN (%d)" % (cfg_name, len(vs)))

        print("")

    print("#define CA_CFG_ID_MAX (%d)" % len(cfg_array))


def dump_hexdump_file(cfg_array):

    pack_types = {
        "INT-ARRAY": '<I',
        "CHAR-ARRAY": '<B',
        "MAC": '<B',
        "CHAR": '<B',
        "INT": '<I',
    }

    o = b''
    for cfg_item in cfg_array:
        (cfg_type, cfg_name, cfg_value) = cfg_item

        if cfg_type == 'STRING':
            a = re.sub(string_quote_regex, "", cfg_value)
            o += a.ljust(special_cases[cfg_name], '\0').encode()
        else:
            a = re.sub(brace_regex, "", cfg_value)
            struct_type = pack_types[cfg_type]
            base = 0 if cfg_type != "MAC" else 16
            for v in a.split(delimeters[cfg_type]):
                o += pack(struct_type, int(v.strip(), base))

    print("static ca_uint8_t scfg_values_mem[] = {")
    s = ""
    for i, v in enumerate(o, 1):
        s += "0x%02x, " % v
        if (i % 8) == 0:
            print(s)
            s = ""
    print(s)
    print("};")


def parse_cfg_line(line):

    cfg_type = None
    for atype in support_types:
        if line.startswith(atype):
            cfg_type = atype
            break
    else:
        print("type Not found")
        return None

    cfg_type_len = len(cfg_type)
    equal_index = line[cfg_type_len:].find('=')
    if equal_index == -1:
        print("'=' Not found")
        return None

    if line[-1] != ';':
        print("';' Not found")
        return None

    cfg_name = line[cfg_type_len:cfg_type_len + equal_index].strip()
    cfg_value = line[cfg_type_len + equal_index + 1:-1].strip()
    return (cfg_type, cfg_name, cfg_value)


def parse_file(input_file):
    if not os.path.exists(input_file):
        print("file is not existed:%s" % input_file)
        return None

    comments_regex = re.compile(r"[\[#].*$")

    cfg_array = []

    with open(input_file, "rb") as in_fd:
        for line in in_fd:
            line = line.decode()
            line = re.sub(comments_regex, "", line)
            line = line.expandtabs().strip()
            if not line:
                continue

            cfg_item = parse_cfg_line(line)
            if cfg_item == None:
                print(line)
                return None

            cfg_array.append(cfg_item)

    return cfg_array


def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "f:h:dab")
    except getopt.GetoptError as err:
        usage(sys.argv[0])
        sys.exit(0)

    input_file = ""
    def_dump = 0
    big_array_dump = 0
    hexvalue_dump = 0
    for o, a in opts:
        if o == "-f":
            input_file = a
        elif o == "-h":
            print("usage")
        elif o == '-d':
            def_dump = 1
        elif o == '-b':
            big_array_dump = 1
        elif o == '-a':
            hexvalue_dump = 1

    cfg_array = parse_file(input_file)
    if cfg_array == None:
        sys.exit(1)

    sorted_cfg_array = sorted(cfg_array, key=itemgetter(1))

    if def_dump :
        dump_def_file(sorted_cfg_array)

    if big_array_dump :
        dump_bigarray_file(sorted_cfg_array)

    if hexvalue_dump:
        dump_hexdump_file(sorted_cfg_array)


if __name__ == "__main__":
    main()
