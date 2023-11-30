#!/usr/bin/python3

import json
import argparse
import logging
import sys
import numpy as np
import os.path

###############################################################################
def work_on_data(args,data):

    # memory=np.array([], dtype=np.int8)
   
    # first check the global section
    if "start" in data:
        start_adress=int(data["start"], 16)
    else:
        start_adress=0 

    # end_adress = -1 means, memory will grow automatically
    if "size" in data:
        end_adress=int(data["size"], 16)
        end_adress+=start_adress
        #memory = np.arange(end_adress-start_adress, dtype=np.int8)
        memory=np.empty((end_adress-start_adress)+1,dtype=np.int8)
    else:
        memory=np.empty(1,dtype=np.int8)
        end_adress=-1 

    # check if we have a fillbyte defined
    if "fill" in data:
        fill_byte=int(data["fill"], 16)        
        fill_section(memory,start_adress,start_adress,end_adress,fill_byte)
    else:
        fill_byte=0 
    
    
    if not "sections" in data:
       logging.error("no sections defined in json")
       return memory

    start_adress_section=start_adress

    for section in data["sections"]:

      # check if we have a defined start-address. Otherwise append  
      if "start" in section:      
        start_adress_section=int(section["start"],16)
      else:
        start_adress_section=end_adress_section

      logging.info("working on section " + section["name"] + " start at" + hex(start_adress_section) ) 
      
      if "file" in section:  
        end_adress_section=load_section(memory,start_adress,start_adress_section,start_adress_section+int(section["size"],16),section["file"],args)
      if "fill" in section:
        fill_byte=int(section["fill"], 16)        
        fill_section(memory,start_adress,end_adress_section,start_adress_section+int(section["size"],16),fill_byte)
  
    #memory.tofile("bla.bin")
    return memory


###############################################################################
def fill_section(memory,global_start_adress,start_adress,end_adress,fill_byte):

    
    if end_adress < start_adress:
        logging.warning ("adress is wrong , ignoring section")
        return
    
    logging.info("Filling from " + hex(start_adress) + " to " + hex(end_adress) + " with " + hex (fill_byte))    


    # Map the memory-field to offset 0
    for i in range(start_adress-global_start_adress,end_adress-global_start_adress):
        memory.flat[i]=fill_byte
        #np.put(memory,i,fill_byte)
    
###############################################################################
def load_section(memory,global_start_adress,start_adress,end_adress,filename,args):

    
    if end_adress < start_adress:
        logging.warning ("adress is wrong , ignoring section")
        return -1
    
    logging.info("Loading "+ filename + " from " + hex(start_adress) )    
    
    found=False
    i=int(start_adress-global_start_adress)
    for searchdir in args.searchdir:
        logging.debug("Searching in " + searchdir)
        if os.path.exists(searchdir+"/"+filename):
            
            logging.debug("Found file in " + searchdir)
            with open(searchdir+"/"+filename,"rb") as infile:
                found=True
                # read until end of file
                while True:
                    inbyte = infile.read(1)
                    if (inbyte != b""):
                        memory.flat[i]=ord(inbyte)
                    else:
                        return global_start_adress+i
                    i +=1
                    # Check if file runs over the end_adress
                    if i > end_adress:
                        return global_start_adress+i

    logging.error("Can't open file " + filename)
    # Can't load should we ignore this ?
    #sys.exit (-3)
    return start_adress

        

###############################################################################
def open_json(args):

    logging.info('Opening file: ' + str(args.infile))  

    try: 
        with open(args.infile, 'r') as f:
            data = json.load(f)
       
    except:
        logging.error("inputfile found but cannot open")
        sys.exit (-1)



    if args.verbose:
        print(json.dumps(data,indent = 4))

    f.close()

    return data

###############################################################################
def open_outfile(args,data):

    
    if "output" in data:
        outfilename = str(args.outdir)+'/' + data["output"]
    else:
        outfilename="rom.bin"    
    
    logging.info('Writing to ouputfile: ' + outfilename)  

    try:
        f=open(outfilename, "wb")           
       
    except:
        logging.error("Can't open outfile")
        sys.exit(-2)

    return f


def main():
    
    
    parser = argparse.ArgumentParser()
    parser.add_argument('--verbose','-v',action='store_true',help="Extend debug messages")
    parser.add_argument('--quiet','-q',action='store_true',help="print out nothing, good for buildsystems")
    parser.add_argument('--infile','-i',help="Name of the json-file to parse",required=True)
    parser.add_argument('--outdir','-o',help="optional name to store output file",default=".")
    parser.add_argument('--searchdir','-s',nargs='+',help="optional names of directories to search for files (space separated)",default=".",type=str)
    args = parser.parse_args()

    if args.verbose:
        logging.basicConfig(format='bin_merge: %(levelname)s: %(message)s',level=logging.DEBUG)
    if not args.quiet:
        logging.basicConfig(format='bin_merge: %(levelname)s: %(message)s',level=logging.INFO)
    else:
        logging.basicConfig(format='bin_merge: %(levelname)s: %(message)s')


    logging.info("-----------------------------------------------------------------------")
    logging.info("| Bensons bin-merger for creating rom-dumps from multiple input files |")
    logging.info("-----------------------------------------------------------------------")


    data = open_json(args)
    outfile = open_outfile(args,data)
    memory=work_on_data(args,data)
    
    #print (memory)
    memory.tofile(outfile)
    outfile.close()
    
if __name__ == "__main__":
    main()
