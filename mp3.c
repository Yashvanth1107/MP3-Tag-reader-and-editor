#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>  // Developed by user strcasecmp()

typedef struct{
    char title[100];
    char artist[100];
    char album[100];
    char year[10];
    char comment[200];
    char genre[50];
} TagInfo;

// Function Prototypes

int read_size(unsigned char size_bytes[4]);
int validate_mp3(const char *filename);
void view_tags(const char *filename);
void edit_tags(const char *filename, const char *tag, const char *new_value);

// Extracting the tag size from ID3v2 bytes

int read_size(unsigned char size_bytes[4])
{
    return (size_bytes[0] << 21) | (size_bytes[1] << 14) | (size_bytes[2] << 7)  | (size_bytes[3]);
}

// Validating the file extension & checking for ID3 header

int validate_mp3(const char *filename)
{
    const char *ext = strrchr(filename, '.');
    if(!ext || strcmp(ext, ".mp3") != 0)
    {
        printf("Error: '%s' is not an MP3 file!\n", filename);
        return 0;
    }

    FILE *fp = fopen(filename, "rb");
    if(fp == NULL)
    {
        printf("Error: Cannot open file '%s'\n", filename);
        return 0;
    }

    unsigned char header[3];
    if(fread(header, 1, 3, fp) != 3)
    {
        fclose(fp);
        printf("Error: Failed to read header!\n");
        return 0;
    }
    fclose(fp);

    if(strncmp((char*)header, "ID3", 3) != 0)
    {
        printf("Error: No ID3v2 tag found in '%s'\n", filename);
        return 0;
    }

    return 1;
}

// View tag details

void view_tags(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if(fp == NULL)
    {
        printf("Error: Cannot open file '%s'\n", filename);
        return;
    }

    TagInfo tag = {"", "", "", "", "", ""};
    unsigned char header[10];

    if (fread(header, 1, 10, fp) != 10) 
    {
        fclose(fp);
        printf("Error: Failed to read ID3 header\n");
        return;
    }

    while(1)
    {
        char frame_id[5] = {0};
        unsigned char size_bytes[4];
        unsigned short flags;

        if(fread(frame_id, 1, 4, fp) != 4) 
        {
            break;
        }
        if(frame_id[0] == 0) 
        {
            break;
        }
        if(fread(size_bytes, 1, 4, fp) != 4) 
        {
            break;
        }
        if(fread(&flags, 1, 2, fp) != 2) 
        {
            break;
        }

        int size = read_size(size_bytes);
        if(size <= 0) 
        {
            break;
        }
        char *data = (char*)malloc(size + 1);
        if(!data) 
        { 
            fclose(fp); 
            return; 
        }

        if(fread(data, 1, size, fp) != (size_t)size) 
        { 
            free(data);
            break; 
        }
        data[size] = '\0';

        // Decoding the known metadata

        int offset = (data[0] == 1) ? 3 : 1; //Using unary operator

        if(strcmp(frame_id, "TIT2") == 0)
        {
            strncpy(tag.title, data + offset, sizeof(tag.title)-1);
        }
        else if(strcmp(frame_id, "TPE1") == 0)
        {
            strncpy(tag.artist, data + offset, sizeof(tag.artist)-1);
        }
        else if(strcmp(frame_id, "TALB") == 0)
        {
            strncpy(tag.album, data + offset, sizeof(tag.album)-1);
        }
        else if(strcmp(frame_id, "TYER") == 0 || strcmp(frame_id, "TDRC") == 0)
        {
            strncpy(tag.year, data + offset, sizeof(tag.year)-1);
        }
        else if(strcmp(frame_id, "COMM") == 0)
        {
            int i = 4;
            while (i < size && data[i] != '\0') 
            {
                i++;
            }
            if (i + 1 < size)
            {
                strncpy(tag.comment, data + i + 1, sizeof(tag.comment)-1);
            }
        }
        else if(strcmp(frame_id, "TCON") == 0)
        {
            strncpy(tag.genre, data + offset, sizeof(tag.genre)-1);
        }
        free(data);
    }

    fclose(fp);

    printf("------------------------------------------------------------\n");
    printf("           MP3 Tag Reader and Editor \n");
    printf("------------------------------------------------------------\n");
    printf("Title     : %s\n", tag.title);
    printf("Artist    : %s\n", tag.artist);
    printf("Album     : %s\n", tag.album);
    printf("Year      : %s\n", tag.year);
    printf("Music     : %s\n", tag.genre);
    printf("Comment   : %s\n", tag.comment);
    printf("------------------------------------------------------------\n");
}

// Editing tag details 
void edit_tags(const char *filename, const char *tag, const char *new_value)
{
    FILE *fp = fopen(filename, "r+b");
    if(fp == NULL)
    {
        printf("Error: Cannot open file '%s'\n", filename);
        return;
    }

    unsigned char header[10];
    if(fread(header, 1, 10, fp) != 10) 
    {
        fclose(fp);
        printf("Error: Invalid MP3 header!\n");
        return;
    }

    while(1)
    {
        char frame_id[5] = {0};
        unsigned char size_bytes[4];
        unsigned short flags;

        if(fread(frame_id, 1, 4, fp) != 4) 
        {
            break;
        }
        if(frame_id[0] == 0) 
        {
            break;
        }
        if(fread(size_bytes, 1, 4, fp) != 4) 
        {
            break;
        }
        if(fread(&flags, 1, 2, fp) != 2) 
        {
            break;
        }

        int size = read_size(size_bytes);
        if(size <= 0) 
        {
            break;
        }

        long pos = ftell(fp);
        if(strcmp(frame_id, tag) == 0)
        {
            unsigned char *buffer = (unsigned char*)malloc(size);
            if(!buffer) 
            { 
                fclose(fp); 
                printf("Memory error\n"); 
                return; 
            }
            if(fread(buffer, 1, size, fp) != (size_t)size) 
            { 
                free(buffer);
                break; 
            }
            if(strcmp(frame_id, "COMM") == 0)
            {
                unsigned char encoding = 0;
                char language[3] = {'e','n','g'};
                if(size >= 4) 
                {
                    encoding = buffer[0];
                    language[0] = buffer[1];
                    language[1] = buffer[2];
                    language[2] = buffer[3];
                }
                int desc_end = 4;
                while(desc_end < size && buffer[desc_end] != '\0') 
                {
                    desc_end++;
                }
                if(desc_end >= size) 
                {
                    desc_end = 4;
                }

                int new_comment_len = strlen(new_value);
                int required = 1 + 3 + 1 + new_comment_len;
                if(required > size)
                {
                    printf("Comment too long! (%d > %d)\n", required, size);
                    free(buffer);
                    fclose(fp);
                    return;
                }

                fseek(fp, pos, SEEK_SET);
                fputc(encoding, fp);
                fwrite(language, 1, 3, fp);
                fputc('\0', fp);
                fwrite(new_value, 1, new_comment_len, fp);

                for(int i = required; i < size; i++) 
                {
                    fputc('\0', fp);
                }

                printf("COMM updated successfully to '%s'\n", new_value);
                free(buffer);
                fclose(fp);
                return;
            }
            else
            {
                unsigned char encoding = (size >= 1) ? buffer[0] : 0;
                int new_len = strlen(new_value);
                int required = 1 + new_len;
                if(required > size)
                {
                    printf("New value too long! (%d > %d)\n", required, size);
                    free(buffer);
                    fclose(fp);
                    return;
                }

                fseek(fp, pos, SEEK_SET);
                fputc(encoding, fp);
                fwrite(new_value, 1, new_len, fp);
                for(int i = required; i < size; i++) 
                {
                    fputc('\0', fp);
                }
                printf("%s updated successfully to '%s'\n", frame_id, new_value);
                free(buffer);
                fclose(fp);
                return;
            }
        }
        fseek(fp, size, SEEK_CUR);
    }

    printf("⚠️ Tag %s not found!\n", tag);
    fclose(fp);
}

// Main Command Line Interface (CLI) handling

int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        printf("Usage:\n");
        printf("View : %s -v <file.mp3>\n", argv[0]);
        printf("Edit : %s -e -t|-a|-l|-y|-c|-g <new_value> <file.mp3>\n", argv[0]);
        return 1;
    }

    if(strcasecmp(argv[1], "-v") == 0)
    {
        if(!validate_mp3(argv[2])) 
        {
            return 1;
        }
        view_tags(argv[2]);
    }
    else if(strcasecmp(argv[1], "-e") == 0)
    {
        if(argc < 5)
        {
            printf("Error: Missing tag value or filename for edit option!\n");
            return 1;
        }

        char *tag = NULL;
        if (strcasecmp(argv[2], "-t") == 0) 
        {
            tag = "TIT2";
        }
        else if (strcasecmp(argv[2], "-a") == 0) 
        {
            tag = "TPE1";
        }
        else if (strcasecmp(argv[2], "-l") == 0) 
        {
            tag = "TALB";
        }
        else if (strcasecmp(argv[2], "-y") == 0) 
        {
            tag = "TYER";
        }
        else if (strcasecmp(argv[2], "-c") == 0) 
        {
            tag = "COMM";
        }
        else if (strcasecmp(argv[2], "-g") == 0) 
        {
            tag = "TCON";
        }
        else
        {
            printf("Invalid tag! Use -t -a -l -y -c -g\n");
            return 1;
        }

        if(!validate_mp3(argv[4])) 
        {
            return 1;
        }

        edit_tags(argv[4], tag, argv[3]);
        printf("\nUpdated Details:\n");
        view_tags(argv[4]);
    }
    else
    {
        printf("Invalid option. Use -v to view or -e to edit.\n");
    }
    return 0;
}