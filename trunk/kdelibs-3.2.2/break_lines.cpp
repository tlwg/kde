#include <break_lines.h>
#include <klibloader.h>
#include "qcstring.h"
#include <qtextcodec.h>
#include <qcleanuphandler.h>


typedef int (*th_brk_def)(const char*, int[], int);
static th_brk_def th_brk;

namespace khtml {
    struct ThaiCache
    {
        ThaiCache() {
            string = 0;
            allocated = 0x400;
            wbrpos = (int *) malloc(allocated*sizeof(int));
            numwbrpos = 0;
            numisbreakable = 0x400;
            isbreakable = (int *) malloc(numisbreakable*sizeof(int));
        }
        ~ThaiCache() {
            free(wbrpos);
            free(isbreakable);
            library->unload();
        }
        const QChar *string;
        int *wbrpos;
        int *isbreakable;
        unsigned int allocated;
        unsigned int numwbrpos,numisbreakable;
        KLibrary *library;
    };
    static ThaiCache *cache = 0;

    void cleanup_thaibreaks()
    {
        delete cache;
    }

    bool isBreakableThai( const QChar *string, const int pos, const int len)
    {
        static QTextCodec *thaiCodec = QTextCodec::codecForMib(2259);

        /* load libthai dynamically */
        if (!th_brk && thaiCodec) {
            KLibLoader *loader = KLibLoader::self();
            KLibrary *lib = loader->library("libthai.so.0");
            if (lib && lib->hasSymbol("th_brk")) {
                th_brk = (th_brk_def) lib->symbol("th_brk");
                cache = new ThaiCache;
                cache->library = lib;
            } else {
                // indication that loading failed and we shouldn't try to load again
                thaiCodec = 0;
                if (lib)
                    lib->unload();
            }
        }

        if (!th_brk)
            return true;

        // build up string of thai chars
        if ( string != cache->string ) {
            //fprintf(stderr,"new string found (not in cache), calling libthai\n");
            QCString cstr = thaiCodec->fromUnicode( QConstString(string,len).string());
            //printf("About to call libthai::th_brk with str: %s, pos: %d",thaiCache->data(),pos);

            cache->numwbrpos = th_brk(cstr.data(), cache->wbrpos, cache->allocated);
            //fprintf(stderr,"libthai returns with value %d\n",numwbrpos);
            if (cache->numwbrpos > cache->allocated) {
                cache->allocated = cache->numwbrpos;
                cache->wbrpos = (int *)realloc(cache->wbrpos, cache->allocated*sizeof(int));
                cache->numwbrpos = th_brk(cstr.data(), cache->wbrpos, cache->allocated);
            }
	    if ( (unsigned int) len > cache->numisbreakable ) {
		cache->numisbreakable=len;
                cache->isbreakable = (int *)realloc(cache->isbreakable, cache->numisbreakable*sizeof(int));
	    }
	    for (int i = 0 ; i < len ; ++i) {
		cache->isbreakable[i] = 0;
	    }
            if ( cache->numwbrpos > 0 ) {
            	for (int i = cache->numwbrpos-1; i >= 0; --i) {
                	cache->isbreakable[cache->wbrpos[i]] = 1;
		}
	    }
            cache->string = string;
        }
	return cache->isbreakable[pos];
    }
}
