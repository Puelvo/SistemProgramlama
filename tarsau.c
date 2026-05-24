#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#define MAX_FILES 32
#define MAX_TOTAL_SIZE (200 * 1024 * 1024) // 200 MB

// Yardımcı fonksiyon prototipleri
void print_usage();
void archive_files(int file_count, char *files[], char *output_file);
void extract_archive(char *archive_file, char *output_dir);

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage();
        return EXIT_FAILURE;
    }

    // -b: Birleştirme (Archive) İşlemi
    if (strcmp(argv[1], "-b") == 0) {
        char *input_files[MAX_FILES];
        int file_count = 0;
        char *output_file = "a.sau"; // Varsayılan çıktı dosyası adı

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0) {
                if (i + 1 < argc) {
                    output_file = argv[i + 1];
                    break; // -o'dan sonrasını dosya olarak alma
                } else {
                    fprintf(stderr, "Hata: -o parametresinden sonra dosya adi belirtilmedi.\n");
                    return EXIT_FAILURE;
                }
            } else {
                if (file_count >= MAX_FILES) {
                    fprintf(stderr, "Hata: En fazla %d giris dosyasi belirtilebilir.\n", MAX_FILES);
                    return EXIT_FAILURE;
                }
                input_files[file_count++] = argv[i];
            }
        }

        if (file_count == 0) {
            fprintf(stderr, "Hata: Birlestirilecek giris dosyasi belirtilmedi.\n");
            return EXIT_FAILURE;
        }

        archive_files(file_count, input_files, output_file);
    } 
    // -a: Açma (Extract) İşlemi
    else if (strcmp(argv[1], "-a") == 0) {
        if (argc > 4) {
            fprintf(stderr, "Hata: -a parametresi en fazla 2 ek arguman alabilir.\n");
            return EXIT_FAILURE;
        }
        
        char *archive_file = argv[2];
        char *output_dir = (argc == 4) ? argv[3] : "."; // Dizin verilmezse mevcut dizin

        // .sau uzantısı kontrolü
        char *ext = strrchr(archive_file, '.');
        if (!ext || strcmp(ext, ".sau") != 0) {
            fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
            return EXIT_FAILURE;
        }

        extract_archive(archive_file, output_dir);
    } 
    else {
        print_usage();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void print_usage() {
    printf("Kullanim:\n");
    printf("  Birlestirme: ./tarsau -b dosya1 dosya2 ... [-o arsiv.sau]\n");
    printf("  Cikartma:    ./tarsau -a arsiv.sau [hedef_dizin]\n");
}

void archive_files(int file_count, char *files[], char *output_file) {
    printf("Arsivleme islemi baslatiliyor...\n");
    printf("Hedef Dosya: %s\n", output_file);
    printf("Secilen Dosya Sayisi: %d\n", file_count);

}

void extract_archive(char *archive_file, char *output_dir) {
    printf("Cikartma islemi baslatiliyor...\n");
    printf("Arsiv Dosyasi: %s\n", archive_file);
    printf("Hedef Dizin: %s\n", output_dir);

}

// ASCII mi degil mi
bool is_text_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) return false;
    
    int ch;

    while ((ch = fgetc(file)) != EOF) {
        if (ch == 0 || (ch > 127)) { 
            fclose(file);
            return false;
        }
    }
    
    fclose(file);
    return true;
}

void archive_files(int file_count, char *files[], char *output_file) {
    struct stat st;
    long total_size = 0;
    char metadata[8192] = ""; 
    

    for (int i = 0; i < file_count; i++) {
        // sadece metin dosyası 
        if (!is_text_file(files[i])) {
            printf("%s giriş dosyasının formatı uyumsuzdur!\n", files[i]); // 
            exit(EXIT_SUCCESS);
        }
        
        if (stat(files[i], &st) == 0) {
            total_size += st.st_size;
            
            //boyut kontrolü 
            if (total_size > MAX_TOTAL_SIZE) {
                fprintf(stderr, "Hata: Giris dosyalarinin toplam boyutu 200 MB'i gecemez.\n");
                exit(EXIT_FAILURE);
            }
            

            char record[256];

            snprintf(record, sizeof(record), "|%s,%04o,%ld|", files[i], st.st_mode & 0777, st.st_size);
            strcat(metadata, record);
        } else {
            perror("Dosya bilgileri okunamadi");
            exit(EXIT_FAILURE);
        }
    }

    // arşiv dosyası
    FILE *out = fopen(output_file, "w");
    if (!out) {
        perror("Arsiv dosyasi olusturulamadi");
        exit(EXIT_FAILURE);
    }

    // Organizasyon bölümünün boyutunu hesapla ve ilk 10 bayta yaz 
    int metadata_len = strlen(metadata);
    fprintf(out, "%010d", metadata_len); 
    
    // Organizasyon verisini yaz
    fprintf(out, "%s", metadata);

    // Dosya içeriklerini art arda (ayırıcı olmadan) ekle 
    for (int i = 0; i < file_count; i++) {
        FILE *in = fopen(files[i], "r");
        if (in) {
            char buffer[4096];
            size_t bytes_read;
            while ((bytes_read = fread(buffer, 1, sizeof(buffer), in)) > 0) {
                fwrite(buffer, 1, bytes_read, out);
            }
            fclose(in);
        }
    }

    fclose(out);
    printf("Dosyalar birlestirildi.\n");
}
void extract_archive(char *archive_file, char *output_dir) {
    FILE *in = fopen(archive_file, "r");
    if (!in) {
        fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
        exit(EXIT_FAILURE);
    }


    char size_buf[11] = {0};
    if (fread(size_buf, 1, 10, in) != 10) {
        fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
        fclose(in);
        exit(EXIT_FAILURE);
    }

    int metadata_len = atoi(size_buf);
    if (metadata_len <= 0) {
        fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
        fclose(in);
        exit(EXIT_FAILURE);
    }


    char *metadata = malloc(metadata_len + 1);
    if (!metadata) {
        perror("Bellek ayirma hatasi");
        fclose(in);
        exit(EXIT_FAILURE);
    }

    if (fread(metadata, 1, metadata_len, in) != (size_t)metadata_len) {
        fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
        free(metadata);
        fclose(in);
        exit(EXIT_FAILURE);
    }
    metadata[metadata_len] = '\0'; // String sonlandırıcı ekle

    // Hedef dizin kontrolü ve oluşturulması
    struct stat st = {0};
    if (stat(output_dir, &st) == -1) {
        if (mkdir(output_dir, 0777) != 0) {
            perror("Dizin olusturulamadi");
            free(metadata);
            fclose(in);
            exit(EXIT_FAILURE);
        }
    }

    //parse
    char *token = strtok(metadata, "|");
    while (token != NULL) {
        char filename[256];
        unsigned int perms;
        long size;

        
        if (sscanf(token, "%[^,],%o,%ld", filename, &perms, &size) == 3) {
            // Hedef dizin ile dosya adını birleştir
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", output_dir, filename);

            FILE *out = fopen(filepath, "w");
            if (!out) {
                perror("Dosya olusturulamadi");
                free(metadata);
                fclose(in);
                exit(EXIT_FAILURE);
            }

            // İçeriği kopyala
            char buffer[4096];
            long bytes_remaining
