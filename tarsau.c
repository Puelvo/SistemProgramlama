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
