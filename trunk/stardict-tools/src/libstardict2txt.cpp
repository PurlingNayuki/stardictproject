#include <string.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include <glib.h>

#include <string>

#include "libstardict2txt.h"

static void convert2tabfile(const gchar *ifofilename, print_info_t print_info)
{
	std::string idxfilename=ifofilename;
	idxfilename.replace(idxfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "idx");
	struct stat idx_stats;
	if (g_stat (idxfilename.c_str(), &idx_stats) == -1) {
		gchar *str = g_strdup_printf("File not exist: %s\n", idxfilename.c_str());
		print_info(str);
		g_free(str);
		return;
	}
	std::string dictfilename=ifofilename;
        dictfilename.replace(dictfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "dict");
        struct stat dict_stats;
        if (g_stat (dictfilename.c_str(), &dict_stats) == -1) {
#ifdef _WIN32
		gchar *str = g_strdup_printf("File not exist: %s\nPlease rename somedict.dict.dz to somedict.dict.gz and use SevenZip to uncompress the somedict.dict.gz file, then you can get the somedict.dict file.\n", dictfilename.c_str());
#else
		gchar *str = g_strdup_printf("File not exist: %s\nPlease do \"mv somedict.dict.dz somedict.dict.gz;gunzip somedict.dict.gz\"\n", dictfilename.c_str());
#endif
		print_info(str);
		g_free(str);
                return;
        }
	gchar *idxbuffer = (gchar *)g_malloc (idx_stats.st_size);
	gchar *idxbuffer_end = idxbuffer+idx_stats.st_size;
	FILE *idxfile;
	idxfile = g_fopen(idxfilename.c_str(),"rb");
	fread (idxbuffer, 1, idx_stats.st_size, idxfile);
	fclose (idxfile);

	gchar *dictbuffer = (gchar *)g_malloc (dict_stats.st_size);
        FILE *dictfile;
        dictfile = g_fopen(dictfilename.c_str(),"rb");
        fread (dictbuffer, 1, dict_stats.st_size, dictfile);
        fclose (dictfile);

	gchar *basefilename = g_path_get_basename(ifofilename);
	std::string txtfilename=basefilename;
	g_free(basefilename);
	txtfilename.replace(txtfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "txt");
	gchar *str = g_strdup_printf("Write to file: %s\n", txtfilename.c_str());
	print_info(str);
	g_free(str);
	FILE *txtfile;
	txtfile = g_fopen(txtfilename.c_str(),"w");

	gchar *p=idxbuffer;
	int wordlen;
	guint32 offset, size;
	gchar *data;
	while (1) {
		if (p == idxbuffer_end) {
			break;
		}
		wordlen=strlen(p);
		fwrite(p, wordlen, 1, txtfile);
		fwrite("\t", 1, 1, txtfile);
		p+=wordlen +1;
		offset=*reinterpret_cast<guint32 *>(p);
		offset=g_ntohl(offset);
		p+=sizeof(guint32);
		size=*reinterpret_cast<guint32 *>(p);
		size=g_ntohl(size);
		p+=sizeof(guint32);
		data=dictbuffer+offset;
		while ((guint32)(data-(dictbuffer+offset))<size) {
			switch (*data) {
				case '\n':
					fwrite("\\n", 2, 1, txtfile);
					break;
				case '\\':
					fwrite("\\\\", 2, 1, txtfile);
					break;
				default:
					fwrite(data, 1, 1, txtfile);
					break;
			}
			data++;
		}
		fwrite("\n", 1, 1, txtfile);
	}
	fclose(txtfile);
	g_free(idxbuffer);
	g_free(dictbuffer);
}

void convert_stardict2txt(const char *ifofilename, print_info_t print_info)
{
	gchar *buffer;
	g_file_get_contents(ifofilename, &buffer, NULL, NULL);
	if (!g_str_has_prefix(buffer, "StarDict's dict ifo file\nversion=")) {
		print_info("Error, file version is not 2.4.2\n");
		g_free(buffer);
		return;
	}
	gchar *p1,*p2;

        p1 = buffer;
        p2 = strstr(p1,"\nsametypesequence=");
	if (!p2) {
		print_info("Error, no sametypesequence=\n");
		g_free(buffer);
		return;
	}
	p2 += sizeof("\nsametypesequence=") -1;
	if (g_ascii_islower(*p2) && *(p2+1)=='\n') {
		convert2tabfile(ifofilename, print_info);
	} else {
		print_info("Error, sametypesequence is not m\n");
		g_free(buffer);
		return;
	}
	g_free(buffer);
}
