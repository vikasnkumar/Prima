#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include <usp10.h>
#include "guts.h"
#include "win32\win32guts.h"
#include "Widget.h"

#ifdef __cplusplus
extern "C" {
#endif


#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle

#define GRAD 57.29577951

/* emulate underscore and strikeout because ExtTextOutW with ETO_PDY underlines each glyph separately */
static void
underscore_font( Handle self, int x, int y, int width)
{
	Stylus ss;
	HPEN old = NULL;
	HDC dc = sys ps;
	PDCStylus dcs;
	float c, s;

	bzero(&ss, sizeof(ss));
	ss.pen.lopnStyle   = PS_SOLID;
	ss.pen.lopnWidth.x = 1;
	ss.pen.lopnColor   = sys stylus.pen.lopnColor;

	if ( var font. direction != 0) {
		if ( sys font_sin == sys font_cos && sys font_sin == 0.0 ) {
			sys font_sin = sin( var font. direction / GRAD);
			sys font_cos = cos( var font. direction / GRAD);
		}
		c = sys font_cos;
		s = sys font_sin;
	} else {
		s = 0.0;
		c = 1.0;
	}

	if ( var font. style & fsUnderlined ) {
		int i, Y = 0;
		Point pt[2];

		if ( !is_apt( aptTextOutBaseline))
			Y -= var font. descent;
		if (sys otmsUnderscoreSize > 0) {
			Y -= sys otmsUnderscorePosition;
			ss.pen.lopnWidth.x = sys otmsUnderscoreSize;
		} else
			Y += var font. descent - 1;

		pt[0].x = 0;
		pt[0].y = -Y;
		pt[1].x = width;
		pt[1].y = -Y;
		if ( var font. direction != 0) {
			for ( i = 0; i < 2; i++) {
				float x = pt[i].x * c - pt[i].y * s;
				float y = pt[i].x * s + pt[i].y * c;
				pt[i].x = x + (( x > 0) ? 0.5 : -0.5);
				pt[i].y = y + (( y > 0) ? 0.5 : -0.5);
			}
		}

		dcs = stylus_alloc(&ss);
		old = SelectObject( dc, dcs-> hpen );

		MoveToEx( dc, x + pt[0].x, y - pt[0].y, nil);
		LineTo( dc, x + pt[1].x, y - pt[1].y);
	}

	if ( var font. style & fsStruckOut ) {
		int i, Y = 0;
		Point pt[2];

		if ( !is_apt( aptTextOutBaseline))
			Y -= var font. descent;
		if (sys otmsStrikeoutSize > 0) {
			Y -= sys otmsStrikeoutPosition;
			if ( old == NULL || ss.pen.lopnWidth.x != sys otmsStrikeoutSize ) {
				HPEN curr;
				ss.pen.lopnWidth.x = sys otmsStrikeoutSize;
				dcs = stylus_alloc(&ss);
				curr = SelectObject( dc, dcs-> hpen );
				if ( old == NULL ) old = curr;
			}
		} else
			Y -= var font. ascent / 2;

		pt[0].x = 0;
		pt[0].y = -Y;
		pt[1].x = width;
		pt[1].y = -Y;
		if ( var font. direction != 0) {
			for ( i = 0; i < 2; i++) {
				float x = pt[i].x * c - pt[i].y * s;
				float y = pt[i].x * s + pt[i].y * c;
				pt[i].x = x + (( x > 0) ? 0.5 : -0.5);
				pt[i].y = y + (( y > 0) ? 0.5 : -0.5);
			}
		}

		MoveToEx( dc, x + pt[0].x, y - pt[0].y, nil);
		LineTo( dc, x + pt[1].x, y - pt[1].y);
	}

	SelectObject( dc, old );
}

static int
gp_get_text_width( Handle self, const char* text, int len, int flags);

Bool
apc_gp_text_out( Handle self, const char * text, int x, int y, int len, int flags )
{objCheck false;{
	Bool ok = true;
	HDC ps = sys ps;
	int bk  = GetBkMode( ps);
	int opa = is_apt( aptTextOpaque) ? OPAQUE : TRANSPARENT;
	Bool use_path;

	int div = 32768L / (var font. maximalWidth ? var font. maximalWidth : 1);
	if ( div <= 0) div = 1;
	/* Win32 has problems with text_out strings that are wider than
	32K pixel - it doesn't plot the string at all. This hack is
	although ugly, but is better that Win32 default behaviour, and
	at least can be excused by the fact that all GP spaces have
	their geometrical limits. */
	if ( len > div) len = div;

	if ( flags & toUTF8 ) {
		int mb_len;
		if ( !( text = ( char *) guts.alloc_utf8_to_wchar_visual( text, len, &mb_len))) return false;
		len = mb_len;
	}

	use_path = GetROP2( sys ps) != R2_COPYPEN;
	if ( use_path ) {
		STYLUS_USE_BRUSH( ps);
		BeginPath(ps);
	} else {
		STYLUS_USE_TEXT( ps);
		if ( opa != bk) SetBkMode( ps, opa);
	}

	ok = ( flags & toUTF8 ) ?
		TextOutW( ps, x, sys lastSize. y - y, ( U16*)text, len) :
		TextOutA( ps, x, sys lastSize. y - y, text, len);
	if ( !ok ) apiErr;

	if ( var font. style & (fsUnderlined | fsStruckOut))
		underscore_font( self, x, sys lastSize. y - y, gp_get_text_width( self, text, len, flags));

	if ( use_path ) {
		EndPath(ps);
		FillPath(ps);
	} else {
		if ( opa != bk) SetBkMode( ps, bk);
	}

	if ( flags & toUTF8 ) free(( char *) text);
	return ok;
}}

Bool
apc_gp_glyphs_out( Handle self, PGlyphsOutRec t, int x, int y)
{objCheck false;{
	Bool ok = true;
	HDC ps = sys ps;
	int bk  = GetBkMode( ps);
	int opa = is_apt( aptTextOpaque) ? OPAQUE : TRANSPARENT;
	Bool use_path;

	if ( t->len > 8192 ) t->len = 8192;
	use_path = GetROP2( sys ps) != R2_COPYPEN;
	if ( use_path ) {
		STYLUS_USE_BRUSH( ps);
		BeginPath(ps);
	} else {
		STYLUS_USE_TEXT( ps);
		if ( opa != bk) SetBkMode( ps, opa);
	}

	if ( t-> advances ) {
		#define SZ 1024
		INT dx[SZ], n, *pdx, *pdx2;
		int16_t *goffsets = t->positions;
		uint16_t *advances = t->advances;

		n = t-> len * 2;
		if ( n > SZ) {
			if ( !( pdx = malloc(sizeof(INT) * n)))
				pdx = dx;
		} else
			pdx = dx;
		pdx2 = pdx;
		n /= 2;
		while ( n-- > 0 ) {
			*(pdx2++) = *(goffsets++) + *(advances++);
			*(pdx2++) = *(goffsets++);
		}
		ok = ExtTextOutW(ps, x, sys lastSize. y - y, ETO_GLYPH_INDEX | ETO_PDY, NULL, (LPCWSTR) t->glyphs, t->len, pdx);
		if ( pdx != dx ) free(pdx);
		#undef SZ
	} else
		ok = ExtTextOutW(ps, x, sys lastSize. y - y, ETO_GLYPH_INDEX, NULL, (LPCWSTR) t->glyphs, t->len, NULL);
	if ( !ok ) apiErr;

	if ( var font. style & (fsUnderlined | fsStruckOut))
		underscore_font( self, x, sys lastSize. y - y, gp_get_text_width( self, (const char*)t->glyphs, t->len, toGlyphs));

	if ( use_path ) {
		EndPath(ps);
		FillPath(ps);
	} else {
		if ( opa != bk) SetBkMode( ps, bk);
	}

	return ok;
}}


/* Shaping code was borrowed from pangowin32-shape.c by Red Hat Software and Alexander Larsson */

static WORD
make_langid(const char *lang)
{
#define CASE(t,p,s) if (strcmp(lang, t) == 0) return MAKELANGID (LANG_##p, SUBLANG_##p##_##s)
#define CASEN(t,p) if (strcmp(lang, t) == 0) return MAKELANGID (LANG_##p, SUBLANG_NEUTRAL)

/* Languages that most probably don't affect Uniscribe have been
* left out. Uniscribe is documented to use
* SCRIPT_CONTROL::uDefaultLanguage only to select digit shapes, so
* just leave languages with own digits.
*/

	CASEN ("ar", ARABIC);
	CASEN ("hy", ARMENIAN);
	CASEN ("as", ASSAMESE);
	CASEN ("az", AZERI);
	CASEN ("bn", BENGALI);
	CASE ("zh-tw", CHINESE, TRADITIONAL);
	CASE ("zh-cn", CHINESE, SIMPLIFIED);
	CASE ("zh-hk", CHINESE, HONGKONG);
	CASE ("zh-sg", CHINESE, SINGAPORE);
	CASE ("zh-mo", CHINESE, MACAU);
	CASEN ("dib", DIVEHI);
	CASEN ("fa", FARSI);
	CASEN ("ka", GEORGIAN);
	CASEN ("gu", GUJARATI);
	CASEN ("he", HEBREW);
	CASEN ("hi", HINDI);
	CASEN ("ja", JAPANESE);
	CASEN ("kn", KANNADA);
	CASE ("ks-in", KASHMIRI, INDIA);
	CASEN ("ks", KASHMIRI);
	CASEN ("kk", KAZAK);
	CASEN ("kok", KONKANI);
	CASEN ("ko", KOREAN);
	CASEN ("ky", KYRGYZ);
	CASEN ("ml", MALAYALAM);
	CASEN ("mni", MANIPURI);
	CASEN ("mr", MARATHI);
	CASEN ("mn", MONGOLIAN);
	CASE ("ne-in", NEPALI, INDIA);
	CASEN ("ne", NEPALI);
	CASEN ("or", ORIYA);
	CASEN ("pa", PUNJABI);
	CASEN ("sa", SANSKRIT);
	CASEN ("sd", SINDHI);
	CASEN ("syr", SYRIAC);
	CASEN ("ta", TAMIL);
	CASEN ("tt", TATAR);
	CASEN ("te", TELUGU);
	CASEN ("th", THAI);
	CASE ("ur-pk", URDU, PAKISTAN);
	CASE ("ur-in", URDU, INDIA);
	CASEN ("ur", URDU);
	CASEN ("uz", UZBEK);

#undef CASE
#undef CASEN

	return MAKELANGID (LANG_NEUTRAL, SUBLANG_NEUTRAL);
}

static WORD
langid(const char *lang)
{
#define LANGBUF 5
	static char last_lang[LANGBUF] = "";
	static WORD last_langid = MAKELANGID (LANG_NEUTRAL, SUBLANG_NEUTRAL);

	if ( strncmp( lang, last_lang, LANGBUF) == 0) return last_langid;
	last_langid = make_langid(lang);
	strncpy( last_lang, lang, LANGBUF);
	return last_langid;
#undef LANGBUF
}

#pragma pack(1)
typedef struct {
	HFONT font;
	DWORD script;
} ScriptCacheKey;
#pragma pack()


/* convert UTF-32 to UTF-16, mark surrogates */
static Bool
build_wtext( PTextShapeRec t,
	WCHAR * wtext, unsigned int * wlen,
	unsigned int * first_surrogate_pair, unsigned int ** surrogate_map)
{
	int i;
	unsigned int index = 0, *curr = NULL;
	uint32_t *src;
	WCHAR *dst;

	for ( i = *wlen = 0, src = t->text, dst = wtext; i < t->len; i++, index++) {
		uint32_t c = *src++;
		if ( c >= 0x10000 && c <= 0x10FFFF ) {
			c -= 0x10000;
			*(dst++) = 0xd800 + (c & 0x3ff);
			*(dst++) = 0xdc00 + (c >> 10);
			if ( !*surrogate_map ) {
				*first_surrogate_pair = i;
				*surrogate_map = malloc(sizeof(unsigned int*) * (t-> len - i));
				if ( !*surrogate_map ) return false;
				curr = *surrogate_map;
			}
			*(curr++) = index;
			*(curr++) = index;
			*wlen += 2;
		} else {
			if (( c >= 0xD800 && c <= 0xDFFF ) || ( c > 0x10FFFF )) c = 0;
			if ( *surrogate_map )
				*(curr++) = index;
			*(dst++) = c;
			(*wlen)++;
		}
	}

	return true;
}

static void
fill_null_glyphs( PTextShapeRec t, unsigned int char_pos, unsigned int itemlen, unsigned int * surrogate_map, unsigned int first_surrogate_pair)
{
	int i, nglyphs;
	if ( surrogate_map ) {
		int p1 = char_pos;
		int p2 = p1 + itemlen - 1;
		if ( p1 >= first_surrogate_pair ) p1 = surrogate_map[p1 - first_surrogate_pair];
		if ( p2 >= first_surrogate_pair ) p2 = surrogate_map[p2 - first_surrogate_pair];
		nglyphs = p2 - p1 + 1;
	} else
		nglyphs = itemlen;
	bzero( t->glyphs + t->n_glyphs, sizeof(uint16_t) * nglyphs);
	for ( i = 0; i < nglyphs; i++)
		t->indexes[ t->n_glyphs + i ] = i;
	t-> n_glyphs += nglyphs;
	if ( t->advances ) {
		bzero( t->advances + t->n_glyphs, sizeof(uint16_t) * nglyphs);
		bzero( t->positions + t->n_glyphs, sizeof(uint16_t) * nglyphs * 2);
	}
}

static void
convert_indexes( Bool rtl, unsigned int char_pos, unsigned int itemlen, unsigned int nglyphs, WORD * indexes, uint16_t * out_indexes)
{
	int j, last_glyph, last_char;
	if (rtl) {
		for ( j = last_char = 0, last_glyph = nglyphs - 1; j < itemlen; j++) {
			int k, rlen = 1;
			WORD curr_glyph = indexes[j];
			last_char = j;
			for ( k = j + 1; k < itemlen; k++) {
				if ( indexes[k] == curr_glyph )
					rlen++;
				else
					break;
			}
			for ( ; last_glyph >= curr_glyph; last_glyph--) 
				out_indexes[last_glyph] = j + char_pos;
			j += rlen - 1;
		}
		for ( ; last_glyph >= 0; last_glyph--)
			out_indexes[last_glyph] = last_char + char_pos;
	} else {
		for ( j = last_char = last_glyph = 0; j < itemlen; j++) {
			int k, rlen = 1;
			WORD curr_glyph = indexes[j];
			last_char = j;
			for ( k = j + 1; k < itemlen; k++) {
				if ( indexes[k] == curr_glyph )
					rlen++;
				else
					break;
			}
			for ( ; last_glyph <= curr_glyph; last_glyph++) 
				out_indexes[last_glyph] = j + char_pos;
			j += rlen - 1;
		}
		for ( ; last_glyph < nglyphs; last_glyph++)
			out_indexes[last_glyph] = last_char + char_pos;
	}
}

static Bool
win32_unicode_shaper( Handle self, PTextShapeRec t)
{
	Bool ok = false;
	HRESULT hr;
	WCHAR * wtext = NULL;
	uint16_t * out_indexes;
	int i, item, item_step, nitems;
	SCRIPT_CONTROL control;
	SCRIPT_ITEM *items = NULL;
	WORD *indexes = NULL;
	SCRIPT_VISATTR *visuals = NULL;
	int *advances = NULL;
	GOFFSET *goffsets = NULL;
	unsigned int * surrogate_map = NULL, first_surrogate_pair = 0, wlen;

	if ((items = malloc(sizeof(SCRIPT_ITEM) * (t-> len + 1))) == NULL)
		goto EXIT;
	if ((indexes = malloc(sizeof(WORD) * t-> n_glyphs_max)) == NULL)
		goto EXIT;
	if ((visuals = malloc(sizeof(SCRIPT_VISATTR) * t-> n_glyphs_max)) == NULL)
		goto EXIT;
	if ((wtext = malloc(sizeof(WCHAR) * 2 * t->len)) == NULL)
		goto EXIT;
	if ((advances = malloc(sizeof(int) * t->n_glyphs_max)) == NULL)
		goto EXIT;
	if ((goffsets = malloc(sizeof(GOFFSET) * t->n_glyphs_max)) == NULL)
		goto EXIT;

	build_wtext( t, wtext, &wlen, &first_surrogate_pair, &surrogate_map);

	bzero(&control, sizeof(control));
	control.uDefaultLanguage = t->language ?
		langid(t->language) :
		MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

	if ((hr = ScriptItemize(wtext, wlen, t->n_glyphs_max, &control, NULL, items, &nitems)) != S_OK) {
		apiHErr(hr);
		goto EXIT;
	}
#ifdef _DEBUG
	printf("itemizer: %d\n", nitems);
#endif

	if ( t->flags & toRTL ) {
		item = nitems - 1;
		item_step = -1;
	} else {
		item = 0;
		item_step = 1;
	}

	for (
		i = 0, out_indexes = t-> indexes;
		i < nitems;
		i++, item += item_step
	) {
		int j, itemlen, nglyphs;

		SCRIPT_CACHE * script_cache;
		ScriptCacheKey key = { sys fontResource->hfont, items[item].a.eScript };
		if (( script_cache = ( SCRIPT_CACHE*) hash_fetch( scriptCacheMan, &key, sizeof(key))) == NULL) {
			if ((script_cache = malloc(sizeof(SCRIPT_CACHE))) == NULL)
				goto EXIT;
			*script_cache = NULL;
			hash_store( scriptCacheMan, &key, sizeof(key), script_cache);
		}

		itemlen = items[item+1].iCharPos - items[item].iCharPos;
		items[item].a.fRTL = (t->flags & toRTL) ? 1 : 0;
		//printf("shape(%d @ %d) len %d %s\n", item, items[item].iCharPos, itemlen, items[item].a.fRTL ? "RTL" : "LTR");
		if (( hr = ScriptShape(
			sys ps, script_cache,
			wtext + items[item].iCharPos, itemlen, t->n_glyphs_max,
			&items[item].a,
			t->glyphs + t->n_glyphs, indexes, visuals,
			&nglyphs
		)) != S_OK) {
			if ( hr == USP_E_SCRIPT_NOT_IN_FONT) {
#ifdef _DEBUG
				printf("USP_E_SCRIPT_NOT_IN_FONT\n");
#endif
				fill_null_glyphs(t, items[item].iCharPos, itemlen, surrogate_map, first_surrogate_pair);
				continue;
			}
			apiHErr(hr);
			goto EXIT;
		}
#ifdef _DEBUG
		{
			int i;
			printf("shape input %d: ", item);
			for ( i = 0; i < itemlen; i++) {
				printf("%x ", *(wtext + items[item].iCharPos + i));
			}
			printf("\n");
			printf("shape output: ");
			for ( i = 0; i < nglyphs; i++) {
				printf("%d(%x) ", indexes[i], t->glyphs[t->n_glyphs + i]);
			}
			printf("\n");
			printf("indexes: ");
			for ( i = 0; i < nglyphs; i++) {
				printf("%d ", out_indexes[i]);
			}
			printf("\n");
		}
#endif

		convert_indexes( items[item].a.fRTL, items[item].iCharPos, itemlen, nglyphs, indexes, out_indexes);

		/* map from utf16 */
		if ( surrogate_map ) {
			int k;
			uint16_t * out_glyphs = t-> glyphs + t-> n_glyphs;
			for ( j = k = 0; j < nglyphs; j++, k++) {
				if ( k < j)
					out_glyphs[k] = out_glyphs[j];
				if ( out_indexes[j] >= first_surrogate_pair)
					out_indexes[k] = surrogate_map[out_indexes[j] - first_surrogate_pair];
				else if ( k < j )
					out_indexes[k] = out_indexes[j];
			}
		}

		if ( t-> advances ) {
			GOFFSET * i_g;
			int * i_a, i;
			ABC abc;
			int16_t * o_g;
			uint16_t *o_a;

			if (( hr = ScriptPlace(sys ps, script_cache, t->glyphs + t->n_glyphs, nglyphs,
			       visuals, &items[item].a,
			       advances, goffsets, &abc)) != S_OK
			) {
				apiHErr(hr);
				goto EXIT;
			}
			for (
				i = 0,
				i_a = advances,
				i_g = goffsets,
				o_a = t->advances  + t-> n_glyphs,
				o_g = t->positions + t-> n_glyphs * 2;
				i < nglyphs;
				i++
			) {
				*(o_a++) = *(i_a++);
				*(o_g++) = i_g->dv;
				*(o_g++) = i_g->du;
				i_g++;
			}
		}

		t-> n_glyphs += nglyphs;
		out_indexes += nglyphs;
	}

	ok = true;

EXIT:
	if ( surrogate_map  ) free(surrogate_map);
	if ( goffsets ) free(goffsets);
	if ( advances ) free(advances);
	if ( indexes )  free(indexes);
	if ( visuals  ) free(visuals);
	if ( items    ) free(items);
	if ( wtext    ) free(wtext);
	return ok;
}

static Bool
win32_mapper( Handle self, PTextShapeRec t, Bool unicode)
{
	int i, len = t->len;
	uint32_t *src = t-> text;
	uint16_t *glyphs = t->glyphs;
	INT buf[8192];
	DWORD ret;

	if ( len > 8192 ) len = 8192;

	if ( unicode ) {
		char *dst = (char*) buf;
		for ( i = 0; i < t->len; i++)
			*(dst++) = *(src++);
		ret = GetGlyphIndicesA( sys ps, (LPCSTR)buf, t->len, t->glyphs, GGI_MARK_NONEXISTING_GLYPHS);
	} else {
		WCHAR *dst = (WCHAR*) buf;
		for ( i = 0; i < t->len; i++)
			*(dst++) = *(src++);
		ret = GetGlyphIndicesW( sys ps, (LPCWSTR)buf, t->len, t->glyphs, GGI_MARK_NONEXISTING_GLYPHS);
	}
	if ( ret == GDI_ERROR)
		apiErrRet;
	t-> n_glyphs = ret;
	for ( i = 0; i < t->n_glyphs; i++, glyphs++)
		if (*glyphs == 0xffff) *glyphs = 0;

	if ( t->advances ) {
		INT *widths = buf;
		uint16_t *advances = t->advances;
		bzero(t->positions, t->n_glyphs * sizeof(int16_t) * 2);
		if ( GetCharWidthI(sys ps, 0, t->n_glyphs, t->glyphs, buf) == 0)
			apiErrRet;
		for ( i = 0; i < t->n_glyphs; i++, widths++)
			*(advances++) = (*widths >= 0) ? *widths : 0;
	}

	return true;
}

static Bool
win32_byte_mapper( Handle self, PTextShapeRec t)
{
	return win32_mapper(self, t, false);
}

static Bool
win32_unicode_mapper( Handle self, PTextShapeRec t)
{
	return win32_mapper(self, t, true);
}

PTextShapeFunc
apc_gp_get_text_shaper( Handle self, int * type)
{
	if ( *type == tsBytes ) {
		*type = (sys tmPitchAndFamily & TMPF_TRUETYPE) ?
			tsGlyphs :
			tsNone;
		return win32_byte_mapper;
	} else if ( *type == tsGlyphs ) {
		*type = (sys tmPitchAndFamily & TMPF_TRUETYPE) ?
			tsGlyphs :
			tsNone;
		return win32_unicode_mapper;
	} else {
		*type = (sys tmPitchAndFamily & TMPF_TRUETYPE) ?
			tsFull :
			tsNone;
		return win32_unicode_shaper;
	}
}

#define TM(field) to->field = from->field
void
textmetric_c2w( LPTEXTMETRICA from, LPTEXTMETRICW to)
{
	TM(tmHeight);
	TM(tmAscent);
	TM(tmDescent);
	TM(tmInternalLeading);
	TM(tmExternalLeading);
	TM(tmAveCharWidth);
	TM(tmMaxCharWidth);
	TM(tmWeight);
	TM(tmOverhang);
	TM(tmDigitizedAspectX);
	TM(tmDigitizedAspectY);
	TM(tmFirstChar);
	TM(tmLastChar);
	TM(tmDefaultChar);
	TM(tmBreakChar);
	TM(tmItalic);
	TM(tmUnderlined);
	TM(tmStruckOut);
	TM(tmPitchAndFamily);
	TM(tmCharSet);
}
#undef TM

PFontABC
apc_gp_get_font_abc( Handle self, int first, int last, int flags)
{objCheck NULL;{
	int i;
	PFontABC  f1;
	ABCFLOAT *f2 = NULL;
	ABC *f3 = NULL;

	if ( flags & toGlyphs ) {
		if ( !(f3 = ( ABC*) malloc(( last - first + 1) * sizeof( ABC))))
			return NULL;
	} else {
		if ( !( f2 = ( ABCFLOAT*) malloc(( last - first + 1) * sizeof( ABCFLOAT))))
			return NULL;
	}

	if ( !( f1 = ( PFontABC) malloc(( last - first + 1) * sizeof( FontABC)))) {
		if ( f2 ) free(f2);
		if ( f3 ) free(f3);
		return NULL;
	}


	if ( flags & toGlyphs ) {
		if (!GetCharABCWidthsI( sys ps, first, last - first + 1, NULL, f3)) apiErr;
	} else if ( flags & toUnicode ) {
		if (!GetCharABCWidthsFloatW( sys ps, first, last, f2)) apiErr;
	} else {
		if (!GetCharABCWidthsFloatA( sys ps, first, last, f2)) apiErr;
	}

	for ( i = 0; i <= last - first; i++) {
		f1[i].a = f2 ? f2[i].abcfA : f3[i].abcA;
		f1[i].b = f2 ? f2[i].abcfB : f3[i].abcB;
		f1[i].c = f2 ? f2[i].abcfC : f3[i].abcC;
	}

	if ( f2) free( f2);
	if ( f3) free( f3);
	return f1;
}}

/* extract vertical data from a bitmap */
Bool
gp_get_font_def_bitmap( Handle self, int first, int last, int flags, PFontABC abc)
{
	Bool ret = true;
	Font font;
	int i, j, h, w, lineSize;
	HBITMAP bm, oldBM;
	BITMAPINFO bi;
	HDC dc;
	LOGFONT logfont;
	HFONT hfont, oldFont;
	Byte * glyph, * empty;

	/* don't to need exact dimension, just to fit the glyph is enough */
	w = var font. maximalWidth * 2;
	h = var font. height;
	lineSize = (( w + 31) / 32) * 4;
	w = lineSize * 8;
	if ( !( glyph = malloc( lineSize * ( h + 1 ))))
		return false;
	empty = glyph + lineSize * h;
	memset( empty, 0xff, lineSize);

	if ( !( dc = CreateCompatibleDC( NULL ))) {
		free( glyph );
		return false;
	}
	if ( !( bm = CreateBitmap( w, h, 1, 1, NULL))) {
		free( glyph );
		return false;
	}

	bi. bmiHeader. biSize         = sizeof( bi. bmiHeader);
	bi. bmiHeader. biPlanes       = 1;
	bi. bmiHeader. biBitCount     = 1;
	bi. bmiHeader. biSizeImage    = lineSize * h;
	bi. bmiHeader. biWidth        = w;
	bi. bmiHeader. biHeight       = h;
	bi. bmiHeader. biCompression  = BI_RGB;
	bi. bmiHeader. biClrUsed      = 2;
	bi. bmiHeader. biClrImportant = 2;

	oldBM    = SelectObject( dc, bm );

	font = var font;
	font. direction = 0;
	font_font2logfont( &font, &logfont);
	hfont = CreateFontIndirect( &logfont);
	oldFont = SelectObject( dc, hfont );

	memset( abc, 0, sizeof(FontABC) * (last - first + 1));
	for ( i = 0; i <= last - first; i++) {
		Rectangle( dc, -1, -1, w+2, h+2 );
		if ( flags & toGlyphs ) {
			WCHAR ch = first + i;
			ExtTextOutW( dc, var font. maximalWidth, 0, ETO_GLYPH_INDEX, NULL, &ch, 1, NULL);
		} else if ( flags & toUnicode ) {
			WCHAR ch = first + i;
			TextOutW( dc, var font. maximalWidth, 0, &ch, 1);
		} else {
			CHAR ch = first + i;
			TextOutA( dc, var font. maximalWidth, 0, &ch, 1);
		}

		if ( !GetDIBits( dc, bm, 0, h, glyph, &bi, DIB_RGB_COLORS)) {
			ret = false;
			break;
		}

/*
		for ( j = 0; j < h; j++) {
			int k, l;
			for ( k = 0; k < lineSize; k++) {
				Byte * p = glyph + j * lineSize + k;
				printf(".");
				for ( l = 0; l < 8; l++) {
					int z = (*p) & ( 1 << (7-l) );
					printf("%s", z ? "*" : " ");
				}
			}
			printf("\n");
		}
*/

		for ( j = 0; j < h; j++) {
			if ( memcmp( glyph + j * lineSize, empty, lineSize) != 0 ) {
				abc[i]. a = j;
				break;
			}
		}
		for ( j = h - 1; j >= 0; j--) {
			if ( memcmp( glyph + j * lineSize, empty, lineSize) != 0 ) {
				abc[i]. c = h - j - 1;
				break;
			}
		}

		if ( abc[i]. a != 0 || abc[i].c != 0)
			abc[i]. b = h - abc[i]. a - abc[i]. c;
	}

	SelectObject( dc, oldFont);
	SelectObject( dc, oldBM );
	DeleteObject( hfont );
	DeleteObject( bm );
	DeleteDC( dc );
	free( glyph );
	return ret;
}

PFontABC
apc_gp_get_font_def( Handle self, int first, int last, int flags)
{objCheck nil;{
	int i;
	DWORD ret;
	PFontABC f1;
	MAT2 gmat = { {0, 1}, {0, 0}, {0, 0}, {0, 1} };
	GLYPHMETRICS g;

	f1 = ( PFontABC) malloc(( last - first + 1) * sizeof( FontABC));
	if ( !f1) return nil;

	for ( i = 0; i <= last - first; i++) {
		memset(&g, 0, sizeof(g));
		if ( flags & toGlyphs ) {
			ret = GetGlyphOutlineW(sys ps, i + first, GGO_METRICS | GGO_GLYPH_INDEX, &g, sizeof(g), NULL, &gmat);
		} else if ( flags & toUnicode ) {
			ret = GetGlyphOutlineW(sys ps, i + first, GGO_METRICS, &g, sizeof(g), NULL, &gmat);
		} else {
			ret = GetGlyphOutlineA(sys ps, i + first, GGO_METRICS, &g, sizeof(g), NULL, &gmat);
		}
		if ( ret == GDI_ERROR ) {
			if ( !gp_get_font_def_bitmap( self, first, last, flags, f1 )) {
				free( f1 );
				return nil;
			}
			return f1;
		}
		f1[i]. a = var font. descent + g.gmptGlyphOrigin. y - g.gmBlackBoxY; /* XXX g.gmCellIncY ? */
		f1[i]. b = g.gmBlackBoxY;
		f1[i]. c = var font.ascent - g.gmptGlyphOrigin. y;
	}

	return f1;
}}

unsigned long *
apc_gp_get_font_ranges( Handle self, int * count)
{objCheck nil;{
	DWORD i, j, size;
	GLYPHSET * gs;
	unsigned long * ret;
	WCRANGE *src;

	*count = 0;
	if (( size = GetFontUnicodeRanges( sys ps, NULL )) == 0) 
		return NULL;
	if (!( gs = malloc(size)))
		apiErrRet;
	gs-> cbThis = size;
	if ( GetFontUnicodeRanges( sys ps, gs ) == 0) {
		free(gs);
		apiErrRet;
	}
	if ( !( ret = malloc(sizeof(unsigned long) * 2 * gs->cRanges))) {
		free(gs);
		return NULL;
	}
	for ( i = j = 0, src = gs-> ranges; i < gs->cRanges; i++, src++) {
		ret[j++] = src->wcLow;
		ret[j++] = src->wcLow + src->cGlyphs - 1;
	}
	*count = j;

	free(gs);
	return ret;
}}

static char *
single_lang(const char * lang1)
{
	char * m;
	int l = strlen(lang1) + 1;
	m = malloc( l + 3 + 1 );
	strcpy( m, "en" );
	strcpy( m + 3, lang1 );
	m[l + 3] = 0;
	return m;
}

typedef struct {
	DWORD v[4];
	const char * str;
} LangId;

static LangId languages[] = {
	{
		{0x00000001,0x00000000,0x00000000,0x00000000},
		"fj ho ia ie io kj kwm ms ng nr om rn rw sn so ss st sw ts uz xh za zu"
	},{
		{0x00000003,0x00000000,0x00000000,0x00000000},
		"aa an ay bi br ch da de en es eu fil fo fur fy gd gl gv ht id is it jv lb li mg nb nds nl nn no oc pap-an pap-aw pt rm sc sg sma smj sq su sv tl vo wa yap"
	},{
		{0x00000007,0x00000000,0x00000000,0x00000000},
		"af ca co crh cs csb et fi fr hsb hu kl ku-tr mt na nso pl se sk smn tk tn tr vot wen wo"
	},{
		{0x20000007,0x00000000,0x00000000,0x00000000},
		"cy ga gn"
	},{
		{0x0000000f,0x00000000,0x00000000,0x00000000},
		"ro"
	},{
		{0x0000001f,0x00000000,0x00000000,0x00000000},
		"az-az sms"
	},{
		{0x0000005f,0x00000000,0x00000000,0x00000000},
		"ee ln"
	},{
		{0x2000005f,0x00000000,0x00000000,0x00000000},
		"ak fat tw"
	},{
		{0x0000006f,0x00000000,0x00000000,0x00000000},
		"nv"
	},{
		{0x2000004f,0x00000000,0x00000000,0x00000000},
		"vi yo"
	},{
		{0x0000020f,0x00000000,0x00000000,0x00000000},
		"mo"
	},{
		{0x00000027,0x00000000,0x00000000,0x00000000},
		"ty"
	},{
		{0x20000003,0x00000000,0x00000000,0x00000000},
		"ast"
	},{
		{0x00000023,0x00000000,0x00000000,0x00000000},
		"qu quz"
	},{
		{0x00000043,0x00000000,0x00000000,0x00000000},
		"shs"
	},{
		{0x20000043,0x00000000,0x00000000,0x00000000},
		"bin"
	},{
		{0x00000005,0x00000000,0x00000000,0x00000000},
		"bs eo hr ki la lg lt lv mh ny sl"
	},{
		{0x20000005,0x00000000,0x00000000,0x00000000},
		"mi"
	},{
		{0x0000000d,0x00000000,0x00000000,0x00000000},
		"kw"
	},{
		{0x0000001d,0x00000000,0x00000000,0x00000000},
		"bm ff"
	},{
		{0x2000001d,0x00000000,0x00000000,0x00000000},
		"ber-dz kab"
	},{
		{0x00000025,0x00000000,0x00000000,0x00000000},
		"haw"
	},{
		{0x00000205,0x00000000,0x00000000,0x00000000},
		"sh"
	},{
		{0x20000001,0x00000000,0x00000000,0x00000000},
		"ig ve"
	},{
		{0x00000009,0x00000000,0x00000000,0x00000000},
		"kr"
	},{
		{0x00000019,0x00000000,0x00000000,0x00000000},
		"ha sco"
	},{
		{0x00000021,0x00000000,0x00000000,0x00000000},
		"sm to"
	},{
		{0x20000041,0x00000000,0x00000000,0x00000000},
		"hz"
	},{
		{0x00000400,0x00000000,0x00000000,0x00000000},
		"hy"
	},{
		{0x00000800,0x00000000,0x00000000,0x00000000},
		"he yi"
	},{
		{0x00002000,0x00000000,0x00000000,0x00000000},
		"ar az-ir fa ks ku-iq ku-ir lah ota pa-pk pes prs ps-af ps-pk sd ug ur"
	},{
		{0x00004000,0x00000000,0x00000000,0x00000000},
		"nqo"
	},{
		{0x00008000,0x00000000,0x00000000,0x00000000},
		"bh bho brx doi hi hne kok mai mr ne sa sat"
	},{
		{0x00018000,0x00000000,0x00000000,0x00000000},
		"mni"
	},{
		{0x00010000,0x00000000,0x00000000,0x00000000},
		"as bn"
	},{
		{0x00020000,0x00000000,0x00000000,0x00000000},
		"pa"
	},{
		{0x00040000,0x00000000,0x00000000,0x00000000},
		"gu"
	},{
		{0x00080000,0x00000000,0x00000000,0x00000000},
		"or"
	},{
		{0x00000204,0x00000000,0x00000000,0x00000000},
		"cv"
	},{
		{0x00100000,0x00000000,0x00000000,0x00000000},
		"ta"
	},{
		{0x00200000,0x00000000,0x00000000,0x00000000},
		"te"
	},{
		{0x00400000,0x00000000,0x00000000,0x00000000},
		"kn"
	},{
		{0x00800000,0x00000000,0x00000000,0x00000000},
		"ml"
	},{
		{0x01000000,0x00000000,0x00000000,0x00000000},
		"th"
	},{
		{0x02000000,0x00000000,0x00000000,0x00000000},
		"lo"
	},{
		{0x04000000,0x00000000,0x00000000,0x00000000},
		"ka"
	},{
		{0x00000000,0x08070000,0x00000000,0x00000000},
		"ja"
	},{
		{0x00000000,0x08010000,0x00000000,0x00000000},
		"zh-hk zh-mo"
	},{
		{0x00000020,0x08000000,0x00000000,0x00000000},
		"zh-cn zh-sg"
	},{
		{0x00000000,0x01100000,0x00000000,0x00000000},
		"ko"
	},{
		{0x00000000,0x28000000,0x00000000,0x00000000},
		"zh-tw"
	},{
		{0x00000080,0x00000000,0x00000000,0x00000000},
		"el"
	},{
		{0x00000000,0x00000000,0x00000040,0x00000000},
		"bo dz"
	},{
		{0x00000000,0x00000000,0x00000080,0x00000000},
		"syr"
	},{
		{0x00000000,0x00000000,0x00000100,0x00000000},
		"dv"
	},{
		{0x00000000,0x00000000,0x00000200,0x00000000},
		"si"
	},{
		{0x00000000,0x00000000,0x00000400,0x00000000},
		"my"
	},{
		{0x00000000,0x00000000,0x00000800,0x00000000},
		"am byn gez sid ti-er ti-et tig wal"
	},{
		{0x00000000,0x00000000,0x00001000,0x00000000},
		"chr"
	},{
		{0x00000000,0x00000000,0x00002000,0x00000000},
		"iu"
	},{
		{0x00000000,0x00000000,0x00010000,0x00000000},
		"km"
	},{
		{0x00000000,0x00000000,0x00020000,0x00000000},
		"mn-cn"
	},{
		{0x00000000,0x00000000,0x00080000,0x00000000},
		"ii"
	},{
		{0x00000200,0x00000000,0x00000000,0x00000000},
		"ab av ba be bg bua ce chm cu ik kaa kk ku-am kum kv ky lez mk mn-mn os ru sah sel sr tg tt tyv uk"
	},{
		{0x00000000,0x00000000,0x00000000,0x00000004},
		"ber-ma"
	}
};

char *
apc_gp_get_font_languages( Handle self)
{objCheck nil;{
	int i, size;
	char * ret, * p;
	FONTSIGNATURE f;
	LangId *lang;

	memset( &f, 0, sizeof(f));
	i = GetTextCharsetInfo( sys ps, &f, 0);
	if ( i == DEFAULT_CHARSET)
		apiErrRet;

	if ( f. fsUsb[0] == 0 && f. fsUsb[1] == 0 && f. fsUsb[2] == 0 && f. fsUsb[3] == 0) {
		switch( i ) {
		case SYMBOL_CHARSET      : return NULL;
		case SHIFTJIS_CHARSET    : return single_lang("ja");
		case HANGEUL_CHARSET     :
		case GB2312_CHARSET      :
		case CHINESEBIG5_CHARSET : return single_lang("zh");
#ifdef JOHAB_CHARSET
		case GREEK_CHARSET       : return single_lang("el");
		case HEBREW_CHARSET      : return single_lang("he");
		case ARABIC_CHARSET      : return single_lang("ar");
		case VIETNAMESE_CHARSET  : return single_lang("vi");
		case THAI_CHARSET        : return single_lang("th");
		case RUSSIAN_CHARSET     : return single_lang("ru");
#endif
		}
		return single_lang("");
	}

	size = 1024;
	if ( !( p = ret = malloc( size )))
		return NULL;
	for ( i = 0, lang = languages; i < sizeof(languages)/sizeof(LangId); i++, lang++) {
		DWORD *a = f.fsUsb;
		DWORD *b = lang->v;
		if (
			((a[0] & b[0]) == b[0]) &&
			((a[1] & b[1]) == b[1]) &&
			((a[2] & b[2]) == b[2]) &&
			((a[3] & b[3]) == b[3])
		) {
			int len = strlen(lang->str) + 1;
			if ( p - ret + len + 1 > size ) {
				char * p2;
				size *= 2;
				if ( !( p2 = realloc(p, size))) {
					free(ret);
					return NULL;
				}
				p   = p2 + (p - ret);
				ret = p2;
			}
			strcpy( p, lang->str );
			p += len;
		}
	}
	*p = 0;

	while ( p > ret ) {
		if (*p == ' ') *p = 0;
		p--;
	}

	return ret;
}}

static int
gp_get_text_width( Handle self, const char* text, int len, int flags)
{
	SIZE  sz;
	int   div, offset = 0, ret = 0;

	objCheck 0;
	if ( len == 0) return 0;

	/* width more that 32K returned incorrectly by Win32 core */
	if (( div = 32768L / ( var font. maximalWidth ? var font. maximalWidth : 1)) == 0)
		div = 1;

	while ( offset < len) {
		int chunk_len = ( offset + div > len) ? ( len - offset) : div;
		if ( flags & toGlyphs) {
			if ( !GetTextExtentPointI( sys ps, ( WCHAR*) text + offset, chunk_len, &sz)) apiErr;
		} else if ( flags & toUnicode) {
			if ( !GetTextExtentPoint32W( sys ps, ( WCHAR*) text + offset, chunk_len, &sz)) apiErr;
		} else {
			if ( !GetTextExtentPoint32( sys ps, text + offset, chunk_len, &sz)) apiErr;
		}
		ret += sz. cx;
		offset += div;
	}

	if ( flags & toAddOverhangs) {
		if ( sys tmPitchAndFamily & TMPF_TRUETYPE) {
			ABC abc[2];
			if ( flags & toGlyphs) {
				WCHAR * t = (WCHAR*) text;
				GetCharABCWidthsI( sys ps, *t, 1, NULL, &abc[0]);
				GetCharABCWidthsI( sys ps, t[len-1], 1, NULL, &abc[1]);
			} else if ( flags & toUnicode) {
				WCHAR * t = (WCHAR*) text;
				if ( guts. utf8_prepend_0x202D ) {
					/* the 1st character is 0x202D, skip it */
					t++;
					len--;
				}
				GetCharABCWidthsW( sys ps, *t, *t, &abc[0]);
				GetCharABCWidthsW( sys ps, t[len-1], t[len-1], &abc[1]);
			} else {
				GetCharABCWidths( sys ps, text[ 0    ], text[ 0    ], &abc[0]);
				GetCharABCWidths( sys ps, text[ len-1], text[ len-1], &abc[1]);
			}
			if ( abc[0]. abcA < 0) ret -= abc[0]. abcA;
			if ( abc[1]. abcC < 0) ret -= abc[1]. abcC;
		}
	}

	return ret;
}

int
apc_gp_get_text_width( Handle self, const char* text, int len, int flags)
{
	int ret;
	flags &= ~toGlyphs;
	if ( flags & toUTF8 ) {
		int mb_len;
		if ( !( text = ( char *) guts.alloc_utf8_to_wchar_visual( text, len, &mb_len))) return 0;
		len = mb_len;
	}
	ret = gp_get_text_width( self, text, len, flags);
	if ( flags & toUTF8)
		free(( char*) text);
	return ret;
}

int
apc_gp_get_glyphs_width( Handle self, PGlyphsOutRec t)
{
	return gp_get_text_width( self, (const char*)t->glyphs, t->len, t->flags | toGlyphs);
}

static void
gp_get_text_box( Handle self, const char * text, int len, int flags, Point * pt)
{
	pt[0].y = pt[2]. y = var font. ascent - 1;
	pt[1].y = pt[3]. y = - var font. descent;
	pt[4].y = pt[0]. x = pt[1].x = 0;
	pt[3].x = pt[2]. x = pt[4].x = gp_get_text_width( self, text, len, flags );

	if ( !is_apt( aptTextOutBaseline)) {
		int i = 4, d = var font. descent;
		while ( i--) pt[ i]. y += d;
	}

	if ( sys tmPitchAndFamily & TMPF_TRUETYPE) {
		ABC abc[2];
		if ( flags & toGlyphs ) {
			WCHAR * t = (WCHAR*) text;
			GetCharABCWidthsI( sys ps, *t, 1, NULL, &abc[0]);
			GetCharABCWidthsI( sys ps, t[len-1], 1, NULL, &abc[1]);
		} else if ( flags & toUTF8 ) {
			WCHAR * t = (WCHAR*) text;
			if ( guts. utf8_prepend_0x202D ) {
				/* the 1st character is 0x202D, skip it */
				t++;
				len--;
			}
			GetCharABCWidthsW( sys ps, *t, *t, &abc[0]);
			GetCharABCWidthsW( sys ps, t[len-1], t[len-1], &abc[1]);
		} else {
			GetCharABCWidths( sys ps, text[ 0    ], text[ 0    ], &abc[0]);
			GetCharABCWidths( sys ps, text[ len-1], text[ len-1], &abc[1]);
		}
		if ( abc[0]. abcA < 0) {
			pt[0].x += abc[0]. abcA;
			pt[1].x += abc[0]. abcA;
		}
		if ( abc[1]. abcC < 0) {
			pt[2].x -= abc[1]. abcC;
			pt[3].x -= abc[1]. abcC;
		}
	}

	if ( var font. direction != 0) {
		int i;
		float s, c;
		if ( sys font_sin == sys font_cos && sys font_sin == 0.0 ) {
			sys font_sin = sin( var font. direction / GRAD);
			sys font_cos = cos( var font. direction / GRAD);
		}
		s = sys font_sin;
		c = sys font_cos;
		for ( i = 0; i < 5; i++) {
			float x = pt[i]. x * c - pt[i]. y * s;
			float y = pt[i]. x * s + pt[i]. y * c;
			pt[i]. x = x + (( x > 0) ? 0.5 : -0.5);
			pt[i]. y = y + (( y > 0) ? 0.5 : -0.5);
		}
	}
}

Point *
apc_gp_get_text_box( Handle self, const char* text, int len, int flags)
{objCheck nil;{
	Point * pt = ( Point *) malloc( sizeof( Point) * 5);
	if ( !pt) return nil;

	memset( pt, 0, sizeof( Point) * 5);

	flags &= ~toGlyphs;
	if ( flags & toUTF8 ) {
		int mb_len;
		if ( !( text = ( char *) guts.alloc_utf8_to_wchar_visual( text, len, &mb_len))) {
			free( pt);
			return nil;
		}
		len = mb_len;
	}
	gp_get_text_box(self, text, len, flags, pt);
	if ( flags & toUTF8 ) free(( char*) text);
	return pt;
}}

Point *
apc_gp_get_glyphs_box( Handle self, PGlyphsOutRec t)
{objCheck nil;{
	Point * pt = ( Point *) malloc( sizeof( Point) * 5);
	if ( !pt) return nil;

	memset( pt, 0, sizeof( Point) * 5);
	gp_get_text_box(self, (const char*)t->glyphs, t->len, toGlyphs, pt);
	return pt;
}}

int
apc_gp_get_glyph_outline( Handle self, int index, int flags, int ** buffer)
{
	int offset, gdi_size, r_size, *r_buf, *r_ptr;
	Byte * gdi_buf;
	GLYPHMETRICS gm;
	MAT2 matrix;
	UINT format;

	*buffer = NULL;
	memset(&matrix, 0, sizeof(matrix));
	matrix.eM11.value = matrix.eM22.value = 1;

	format = GGO_NATIVE;
	if ( flags & ggoGlyphIndex )       format |= GGO_GLYPH_INDEX;
	if (( flags & ggoUseHints ) == 0 ) format |= GGO_UNHINTED;

	gdi_size = (flags & (ggoUnicode | ggoGlyphIndex)) ?
		GetGlyphOutlineW(sys ps, index, format, &gm, 0, NULL, &matrix) :
		GetGlyphOutlineA(sys ps, index, format, &gm, 0, NULL, &matrix);
	if ( gdi_size <= 0 ) {
		if ( gdi_size < 0 ) apiErr;
		return gdi_size;
	}

	if (( gdi_buf = malloc(gdi_size)) == NULL ) {
		warn("Not enough memory");
		return -1;
	}

	if (
		( (flags & (ggoUnicode | ggoGlyphIndex) ) ?
			GetGlyphOutlineW(sys ps, index, format, &gm, gdi_size, gdi_buf, &matrix) :
			GetGlyphOutlineA(sys ps, index, format, &gm, gdi_size, gdi_buf, &matrix)
		) == GDI_ERROR
	) {
		apiErr;
		free(gdi_buf);
		return -1;
	}

	offset = 0;
	r_size = 0;
	while ( offset < gdi_size ) {
		TTPOLYGONHEADER * h = ( TTPOLYGONHEADER*) (gdi_buf + offset);
		unsigned int curve_offset = sizeof(TTPOLYGONHEADER);
		r_size += 2 /* cmd=ggoMove */ + 2 /* x, y */;
		while ( curve_offset < h->cb ) {
			TTPOLYCURVE * c = (TTPOLYCURVE*) (gdi_buf + offset + curve_offset);
			curve_offset += sizeof(WORD) * 2 + c->cpfx * sizeof(POINTFX);
			r_size += 2 /* cmd */ + c->cpfx * 2;
		}
		offset += h->cb;
	}
	if (( r_buf = malloc(r_size * sizeof(int))) == NULL ) {
		warn("Not enough memory");
		free(gdi_buf);
		return -1;
	}
	r_ptr = r_buf;

	offset = 0;
#define PTX(x) (x.value * 64 + x.fract / (0x10000 / 64))
	while ( offset < gdi_size ) {
		TTPOLYGONHEADER * h = ( TTPOLYGONHEADER*) (gdi_buf + offset);
		unsigned int curve_offset = sizeof(TTPOLYGONHEADER);
		*(r_ptr++) = ggoMove;
		*(r_ptr++) = 1;
		*(r_ptr++) = PTX(h->pfxStart.x);
		*(r_ptr++) = PTX(h->pfxStart.y);
		while ( curve_offset < h->cb ) {
			int i;
			TTPOLYCURVE * c = (TTPOLYCURVE*) (gdi_buf + offset + curve_offset);
			switch ( c-> wType ) {
			case TT_PRIM_LINE:
				*(r_ptr++) = ggoLine;
				break;
			case TT_PRIM_QSPLINE:
				*(r_ptr++) = ggoConic;
				break;
			case TT_PRIM_CSPLINE:
				*(r_ptr++) = ggoCubic;
				break;
			default:
				warn("Unknown constant TT_PRIM_%d\n", c->wType);
				free(gdi_buf);
				free(r_buf);
				return 0;
			}
			*(r_ptr++) = c-> cpfx;
			for ( i = 0; i < c-> cpfx; i++) {
				*(r_ptr++) = PTX(c->apfx[i].x);
				*(r_ptr++) = PTX(c->apfx[i].y);
			}
			curve_offset += sizeof(WORD) * 2 + c->cpfx * sizeof(POINTFX);
		}
		offset += h->cb;
	}
	free(gdi_buf);
	*buffer = r_buf;
	return r_size;
}

Bool
apc_gp_get_text_out_baseline( Handle self)
{
	objCheck 0;
	return is_apt( aptTextOutBaseline);
}

Bool
apc_gp_get_text_opaque( Handle self)
{
	objCheck false;
	return is_apt( aptTextOpaque);
}

Bool
apc_gp_set_text_opaque( Handle self, Bool opaque)
{
	objCheck false;
	apt_assign( aptTextOpaque, opaque);
	return true;
}


Bool
apc_gp_set_text_out_baseline( Handle self, Bool baseline)
{
	objCheck false;
	apt_assign( aptTextOutBaseline, baseline);
	if ( sys ps) SetTextAlign( sys ps, baseline ? TA_BASELINE : TA_BOTTOM);
	return true;
}

#ifdef __cplusplus
}
#endif