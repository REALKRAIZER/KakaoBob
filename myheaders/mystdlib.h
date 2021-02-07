int get_str_size(char *str);
char *my_sha256(char *str,size_t n);
char *from_path_to_filename(char *way);//из формата /way/way/filename.расширение извлекаем filename.расширение
char *from_filename_to_extension(char *filename);//из формата filename.расширение извлекаем расширение
int strcmpi(char *str1,char *str2);//сравниваем строки без учета регистра