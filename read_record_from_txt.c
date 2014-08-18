/*
 * txt format spec: 
 * 1) per line as a record 
 * 2) format: "i2c_addr reg_addr reg_value delay;XXX"
 * 2) '#' show the line is invalid
 * 3) use hex
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#define __DEBUG_LOG__
#define MAX_RECORD_LEN		256
#define MAX_ITEM_NUM		4

struct record
{
   unsigned int reg_addr;
   unsigned int reg_value;
   unsigned int delay;
   char i2c_addr;   
};

int is_valid_hex_char(char ch)
{
   int valid = 0;

   if ((ch>='0' && ch<='9') ||
      (ch>='a' && ch<='f') ||
      (ch>='A' && ch<='F'))
      valid = 1;
   else 
      valid = 0;

   return valid;
}

int char_to_int(char ch)
{
    if (!is_valid_hex_char(ch))
	return -1;

    if (ch>='0' && ch<='9')
       return (ch-'0');
    
    else if (ch>='a' && ch<='f')
       return (ch-'a');
 
    else if (ch>='A' && ch<='F')
       return (ch-'A');
}

int string_to_hex(char *string)
{
    char *p_str, *str_t = string;
    int number, i;

    // filter "0x" or "0X"
    if (*str_t == '0' && (*(str_t+1) == 'x' || (*(str_t+1) == 'X')))
       str_t = string + 2;
    else
       str_t = string;

    number = 0;
    for(i=0; *str_t!='\0'; i++)
    {
	if (is_valid_hex_char(*str_t) == -1)
        {
           printf("char is invalid hex char!\n");
           number = 0;
           break;
        }
	number = number * 16 + char_to_int(*(str_t++));
    }

    return number;
}


int get_one_record(char *src, struct record *record)
{
    char *src_t = NULL;
    char *dst_t = NULL;
    char *dst = NULL;
    char i = 0, j = 0;
    char buffer[4][16];

    if (src == NULL || record == NULL)
    	return -1;
  
    // # at start of the line, the line is invalid
    if (*src == '#')
       return 0; 

    src_t = (char *)malloc(MAX_RECORD_LEN);
    
    if (!src_t)
    {
        printf("%s:%d malloc fail\n", __FUNCTION__, __LINE__);
	return -1;
    }

    memset(src_t, 0, MAX_RECORD_LEN);
    memcpy(src_t, src, strlen(src)+1);

    // sepertate flag ;
    dst = strtok(src_t, ";");
    dst_t = dst;

    //get four strings from a line, and remove space\tab
    while ((*dst_t != '\0') && (j < MAX_ITEM_NUM))
    {
       for (i=0; !isspace(*dst_t) && *dst_t!='\0'; i++)
       {
	   buffer[j][i] = *(dst_t++);
       }
       buffer[j][i] = '\0';

#ifdef __DEBUG_LOG__
       printf("buffer[%d][0]=%s\n", j, &buffer[j][0]);
#endif

       while (isspace(*dst_t)) dst_t++;
  
       j++;  
           
    }

    record->i2c_addr = string_to_hex(&buffer[0][0]);
    record->reg_addr = string_to_hex(&buffer[1][0]);
    record->reg_value = string_to_hex(&buffer[2][0]);
    record->delay = string_to_hex(&buffer[3][0]);

#ifdef __DEBUG_LOG__
    printf("i2c_addr  = 0x%x\n", record->i2c_addr);
    printf("reg_addr  = 0x%x\n", record->reg_addr);
    printf("reg_value = 0x%x\n", record->reg_value);
    printf("delay     = 0x%x\n", record->delay);
#endif

    if ((*dst_t != '\0') && (j >= MAX_ITEM_NUM))
    {
       printf("a line parameter too much!\n");
    } 

    free(src_t);

    return 0;
  
}
/*
 * read a line from file
 */
int get_one_line(FILE **fp, char *line_buf)
{
   unsigned int i = 0;
   int ret;
   char *buf_t = line_buf;

   if (fp==NULL || line_buf==NULL)
     return 0;

   memset(line_buf, 0, MAX_RECORD_LEN);

   while(*buf_t != 0x0a && !feof(*fp))
   {
     buf_t = line_buf+i;
     fread(buf_t, sizeof(char), 1, *fp);
     i++;
   }

   if(!feof(*fp))
   {
      // end of 0x0a
      *buf_t = '\0';
      ret = 1;
   }
   else
   {
      // end of EOF
      *(buf_t+1) = '\0';
      ret = 0;
   }

#ifdef __DEBUG_LOG__
   i = 0;
   if (!strlen(line_buf))
      printf("the new line is empty!\n");
   else 
   {   
      printf("the new line is:\n");
      printf("char format:%s\n", line_buf);
      printf("hex format:");
      while(*(line_buf+i)!='\0')  printf("0x%x ", *(line_buf+(i++)));
      printf("\n");
   }
#endif
   
   return ret;

}

 

int main(int argc, char *argv[])
{

    struct record record;
    FILE *fp;
    char *file_path = argv[1];
    char *buffer = NULL;
    unsigned int file_size;
    char dst[MAX_RECORD_LEN];
    char i;
    

    if (argc > 2)
    {
        printf("warning: parameter count too long!\n");
        return -1; 
    }

    if (argc == 1)
    {
	printf("err: parameter count too short!\n");
	return -1;
    }

    buffer = (char*)malloc(4*1024);

    fp = fopen(file_path, "rt+");

    if (!fp)
    {
	printf("err: open %s fail\n",file_path);
        return -1;
    }  

    fseek(fp, 0, SEEK_END);

    file_size = ftell(fp);

#ifdef __DEBUG_LOG__
    printf("file size is %d\n", file_size);
#endif

    buffer = (char *)malloc(MAX_RECORD_LEN);

    if (!buffer)
    {
       printf("malloc memory fail!\n"); 
       fclose(fp);
       return -1;	
    }

    fseek(fp, 0, SEEK_SET);

    while(get_one_line(&fp, buffer))
    {
       if (strlen(buffer))
       { 
         memset(&record, 0, sizeof(struct record));
 	 get_one_record(buffer, &record);
       }
    }

    free(buffer);
    fclose(fp);


}

