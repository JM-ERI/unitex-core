 /*
  * Unitex
  *
  * Copyright (C) 2001-2009 Universit� Paris-Est Marne-la-Vall�e <unitex@univ-mlv.fr>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Lesser General Public
  * License as published by the Free Software Foundation; either
  * version 2.1 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Lesser General Public License for more details.
  *
  * You should have received a copy of the GNU Lesser General Public
  * License along with this library; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
  *
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Unicode.h"
#include "File.h"
#include "Text_tokens.h"
#include "ExtractUnits.h"
#include "Copyright.h"
#include "Error.h"
#include "Snt.h"
#include "getopt.h"


static void usage() {
u_printf("%S",COPYRIGHT);
u_printf("Usage: Extract [OPTIONS] <text>\n"
         "\n"
         "  <text>: the .snt text to extract from the units\n"
         "\n"
         "OPTIONS:\n"
         "  -y/--yes: extract all matching units (default)\n"
         "  -n/--no: extract all unmatching units\n"
         "  -i X/--index=X: the .ind file that describes the concordance. By default,\n"
         "                  X is the concord.ind file located in the text directory.\n"
         "  -o OUT/--output=OUT: the text file where the units will be stored\n"
         "  -h/--help: this help\n"
         "\n"
         "\nExtract all the units that contain (or not) any part of a utterance. The\n"
         "units are supposed to be separated by the symbol {S}.\n");
}


int main_Extract(int argc,char* argv[]) {
if (argc==1) {
   usage();
   return 0;
}

const char* optstring=":yni:o:h";
const struct option_TS lopts[]= {
      {"yes",no_argument_TS,NULL,'y'},
      {"no",no_argument_TS,NULL,'n'},
      {"output",required_argument_TS,NULL,'o'},
      {"index",required_argument_TS,NULL,'i'},
      {"help",no_argument_TS,NULL,'h'},
      {NULL,no_argument_TS,NULL,0}
};
int val,index=-1;
char extract_matching_units=1;
char text_name[FILENAME_MAX]="";
char concord_ind[FILENAME_MAX]="";
char output[FILENAME_MAX]="";
struct OptVars* vars=new_OptVars();
while (EOF!=(val=getopt_long_TS(argc,argv,optstring,lopts,&index,vars))) {
   switch(val) {
   case 'y': extract_matching_units=1; break;
   case 'n': extract_matching_units=0; break;
   case 'o': if (vars->optarg[0]=='\0') {
                fatal_error("You must specify a non empty output file name\n");
             }
             strcpy(output,vars->optarg);
             break;
   case 'i': if (vars->optarg[0]=='\0') {
                fatal_error("You must specify a non empty concordance file name\n");
             }
             strcpy(concord_ind,vars->optarg);
             break;
   case 'h': usage(); return 0;
   case ':': if (index==-1) fatal_error("Missing argument for option -%c\n",vars->optopt);
             else fatal_error("Missing argument for option --%s\n",lopts[index].name);
   case '?': if (index==-1) fatal_error("Invalid option -%c\n",vars->optopt);
             else fatal_error("Invalid option --%s\n",vars->optarg);
             break;
   }
   index=-1;
}

if (output[0]=='\0') {
   fatal_error("You must specify the output text file\n");
}
if (vars->optind!=argc-1) {
   fatal_error("Invalid arguments: rerun with --help\n");
}
strcpy(text_name,argv[vars->optind]);

struct snt_files* snt_files=new_snt_files(text_name);
U_FILE* text=u_fopen(BINARY,snt_files->text_cod,U_READ);
if (text==NULL) {
   error("Cannot open %s\n",snt_files->text_cod);
   return 1;
}
struct text_tokens* tok=load_text_tokens(snt_files->tokens_txt);
if (tok==NULL) {
   error("Cannot load token list %s\n",snt_files->tokens_txt);
   u_fclose(text);
   return 1;
}
if (tok->SENTENCE_MARKER==-1) {
   error("The text does not contain any sentence marker {S}\n");
   u_fclose(text);
   free_text_tokens(tok);
   return 1;
}

if (concord_ind[0]=='\0') {
   char tmp[FILENAME_MAX];
   get_extension(text_name,tmp);
   if (strcmp(tmp,"snt")) {
      fatal_error("Unable to find the concord.ind file. Please explicit it\n");
   }
   remove_extension(text_name,concord_ind);
   strcat(concord_ind,"_snt");
   strcat(concord_ind,PATH_SEPARATOR_STRING);
   strcat(concord_ind,"concord.ind");
}
U_FILE* concord=u_fopen(UTF16_LE,concord_ind,U_READ);
if (concord==NULL) {
   error("Cannot open concordance %s\n",concord_ind);
   u_fclose(text);
   free_text_tokens(tok);
   return 1;
}
U_FILE* result=u_fopen(UTF16_LE,output,U_WRITE);
if (result==NULL) {
   error("Cannot write output file %s\n",output);
   u_fclose(text);
   u_fclose(concord);
   free_text_tokens(tok);
   return 1;
}
free_snt_files(snt_files);
extract_units(extract_matching_units,text,tok,concord,result);
u_fclose(text);
u_fclose(concord);
u_fclose(result);
free_text_tokens(tok);
free_OptVars(vars);
u_printf("Done.\n");
return 0;
}

