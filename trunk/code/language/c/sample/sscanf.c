int main(void)
{
    int var1=0;
    char var2=0;
    int var3=0;
    char str_val1[]="24\n";
    char str_val2[]="32\n";
    char str_val3[]="48\n";
    sscanf(str_val1,"%d",&var1);
    sscanf(str_val2,"%d",&var2);
    sscanf(str_val3,"%d",&var3);
    printf("var1=%d,var2=%d,var3=%d\n",var1,var2,var3);
}
