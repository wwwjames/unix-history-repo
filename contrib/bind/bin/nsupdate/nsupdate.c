#if !defined(lint) && !defined(SABER)
static const char rcsid[] = "$Id: nsupdate.c,v 8.26 2000/12/23 08:14:48 vixie Exp $";
#endif /* not lint */

/*
 * Copyright (c) 1996,1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include "port_before.h"

#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <resolv.h>
#include <res_update.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <isc/dst.h>
#include "port_after.h"
#include "../named/db_defs.h"

/* XXX all of this stuff should come from libbind.a */

/*
 * Map class and type names to number
 */
struct map {
	char    token[10];
	int     val;
};

struct map class_strs[] = {
	{ "in",         C_IN },
	{ "chaos",      C_CHAOS },
	{ "hs",         C_HS },
};
#define M_CLASS_CNT (sizeof(class_strs) / sizeof(struct map))

struct map type_strs[] = {
	{ "a",          T_A },
	{ "ns",         T_NS },
	{ "cname",      T_CNAME },
	{ "soa",        T_SOA },
	{ "mb",         T_MB },
	{ "mg",         T_MG },
	{ "mr",         T_MR },
	{ "null",       T_NULL },
	{ "wks",        T_WKS },
	{ "ptr",        T_PTR },
	{ "hinfo",      T_HINFO },
	{ "minfo",      T_MINFO },
	{ "mx",         T_MX },
	{ "txt",        T_TXT },
	{ "rp",         T_RP },
	{ "afsdb",      T_AFSDB },
	{ "x25",        T_X25 },
	{ "isdn",       T_ISDN },
	{ "rt",         T_RT },
	{ "nsap",       T_NSAP },
	{ "nsap_ptr",   T_NSAP_PTR },
	{ "sig",        T_SIG },
	{ "key",        T_KEY },
	{ "px",         T_PX },
	{ "loc",        T_LOC },
	{ "nxt",        T_NXT },
	{ "eid",        T_EID },
	{ "nimloc",     T_NIMLOC },
	{ "srv",        T_SRV },
	{ "atma",       T_ATMA },
	{ "naptr",      T_NAPTR },
	{ "kx",         ns_t_kx },
	{ "cert",       ns_t_cert },
	{ "aaaa",       ns_t_aaaa },
};
#define M_TYPE_CNT (sizeof(type_strs) / sizeof(struct map))

struct map section_strs[] = {
	{ "zone",	S_ZONE },
	{ "prereq",	S_PREREQ },
	{ "update", 	S_UPDATE },
	{ "reserved",	S_ADDT },
};
#define M_SECTION_CNT (sizeof(section_strs) / sizeof(struct map))

struct map opcode_strs[] = {
	{ "nxdomain",	NXDOMAIN },
	{ "yxdomain",	YXDOMAIN },
	{ "nxrrset", 	NXRRSET },
	{ "yxrrset",	YXRRSET },
	{ "delete",	DELETE },
	{ "add",	ADD },
};
#define M_OPCODE_CNT (sizeof(opcode_strs) / sizeof(struct map))

static int getcharstring(char *, char *, int, int, int);
static char *progname;

static void usage(void);
static int getword_str(char *, int, char **, char *);

static struct __res_state res;

int dns_findprimary (res_state, char *, struct ns_tsig_key *, char *,
                     int, struct in_addr *);

/*
 * format of file read by nsupdate is kept the same as the log
 * file generated by updates, so that the log file can be fed
 * to nsupdate to reconstruct lost updates.
 * 
 * file is read on line at a time using fgets() rather than
 * one word at a time using getword() so that it is easy to
 * adapt nsupdate to read piped input from other scripts
 *
 * overloading of class/type has to be deferred to res_update()
 * because class is needed by res_update() to determined the
 * zone to which a resource record belongs
 */
int
main(int argc, char **argv) {
	FILE *fp = NULL;
	char buf[BUFSIZ], buf2[BUFSIZ];
	char dnbuf[MAXDNAME], data[MAXDATA];
	char *r_dname, *cp, *startp, *endp, *svstartp;
	char section[15], opcode[10];
	int i, c, n, n1, inside, lineno = 0, vc = 0,
		debug = 0, r_size, r_section, r_opcode,
		prompt = 0, ret = 0, stringtobin = 0;
	int16_t r_class, r_type;
	u_int32_t r_ttl;
	struct map *mp;
	ns_updrec *rrecp;
	ns_updque listuprec;
	extern int getopt();
	extern char *optarg;
	extern int optind, opterr, optopt;
	ns_tsig_key key;
	char *keyfile=NULL, *keyname=NULL;

	progname = argv[0];

	while ((c = getopt(argc, argv, "dsvk:n:")) != -1) {
		switch (c) {
		case 'v':
			vc = 1;
			break;
		case 'd':
			debug = 1;
			break;
		case 's':
			stringtobin = 1;
			break;
		case 'k': {
			/* -k keydir:keyname */
			char *colon;
   
			if ((colon=strchr(optarg, ':'))==NULL) {
				fprintf(stderr, "key option argument should be keydir:keyname\n");
				exit(1);
			}
			keyname=colon+1;
			keyfile=optarg;
			*colon='\0';
			break;
		}
		case 'n':
			keyname=optarg;
			break;
		default:
			usage();
		}
	}

	INIT_LIST(listuprec);

	if (keyfile) {
#ifdef PARSE_KEYFILE
		if ((fp=fopen(keyfile, "r"))==NULL) {
			perror("open keyfile");
			exit(1);
		}
		/* now read the header info from the file */
		if ((i=fread(buf, 1, BUFSIZ, fp)) < 5) {
			fclose(fp);
                	exit(1);
        	}
		fclose(fp);
		fp=NULL;

		p=buf;

		n=strlen(p);		/* get length of strings */
		n1=strlen("Private-key-format: v");
		if (n1 > n || strncmp(buf, "Private-key-format: v", n1)) {
			fprintf(stderr, "Invalid key file format\n");
			exit(1);	/* not a match */
		}
		p+=n1;		/* advance pointer */
		sscanf((char *)p, "%d.%d", &file_major, &file_minor);
		/* should do some error checking with these someday */
		while (*p++!='\n');	/* skip to end of line */

        	n=strlen(p);		/* get length of strings */
        	n1=strlen("Algorithm: ");
        	if (n1 > n || strncmp(p, "Algorithm: ", n1)) {
			fprintf(stderr, "Invalid key file format\n");
                	exit(1);	/* not a match */
		}
		p+=n1;		/* advance pointer */
		if (sscanf((char *)p, "%d", &alg)!=1) {
			fprintf(stderr, "Invalid key file format\n");
			exit(1);
		}
		while (*p++!='\n');	/* skip to end of line */

        	n=strlen(p);		/* get length of strings */
        	n1=strlen("Key: ");
        	if (n1 > n || strncmp(p, "Key: ", n1)) {
			fprintf(stderr, "Invalid key file format\n");
			exit(1);	/* not a match */
		}
		p+=n1;		/* advance pointer */
		pp=p;
		while (*pp++!='\n');	/* skip to end of line, terminate it */
		*--pp='\0';

		key.data=malloc(1024*sizeof(char));
		key.len=b64_pton(p, key.data, 1024);

		strcpy(key.name, keyname);
		strcpy(key.alg, "HMAC-MD5.SIG-ALG.REG.INT");
#else
		/* use the dst* routines to parse the key files
		 * 
		 * This requires that both the .key and the .private files
		 * exist in your cwd, so the keyfile parmeter here is
		 * assumed to be a path in which the K*.{key,private} files
		 * exist.
		 */
		DST_KEY *dst_key;
		char cwd[PATH_MAX+1];

		if (getcwd(cwd, PATH_MAX)==NULL) {
			perror("unable to get current directory");
			exit(1);
		}
		if (chdir(keyfile)<0) {
			fprintf(stderr, "unable to chdir to %s: %s\n", keyfile,
				strerror(errno));
			exit(1);
		}

		dst_init();
		dst_key = dst_read_key(keyname,
				       0 /* not used for private keys */,
				       KEY_HMAC_MD5, DST_PRIVATE);
		if (!dst_key) {
			fprintf(stderr, "dst_read_key: error reading key\n");
			exit(1);
		}
		key.data=malloc(1024*sizeof(char));
		dst_key_to_buffer(dst_key, key.data, 1024);
		key.len=dst_key->dk_key_size;

		strcpy(key.name, keyname);
		strcpy(key.alg, "HMAC-MD5.SIG-ALG.REG.INT");

		if (chdir(cwd)<0) {
			fprintf(stderr, "unable to chdir to %s: %s\n", cwd,
				strerror(errno));
			exit(1);
		}
#endif
	}

	if ((argc - optind) == 0) {
	    /* no file specified, read from stdin */
	    ret = system("tty -s");
	    if (ret == 0) /* terminal */
		prompt = 1;
	    else /* stdin redirect from a file or a pipe */
		prompt = 0;
	} else {
	    /* file specified, open it */
	    /* XXX - currently accepts only one filename */
	    if ((fp = fopen(argv[optind], "r")) == NULL) {
		fprintf(stderr, "error opening file: %s\n", argv[optind]);
		exit (1);
	    }
	}
	for (;;) {

	    inside = 1;
	    if (prompt)
		fprintf(stdout, "> ");
	    if (!fp)
		cp = fgets(buf, sizeof buf, stdin);
	    else
	        cp = fgets(buf, sizeof buf, fp);
	    if (cp == NULL) /* EOF */
		break;
	    lineno++;

	    /* get rid of the trailing newline */
	    n = strlen(buf);
	    buf[--n] = '\0';
 
	    startp = cp;
	    endp = strchr(cp, ';');
	    if (endp != NULL)
		endp--;
	    else
		endp = cp + n - 1;

	    /* verify section name */
	    if (!getword_str(section, sizeof section, &startp, endp)) {
		/* empty line */
		inside = 0;
	    }
	    if (inside) {
		/* inside the same update packet,
		 * continue accumulating records */
		r_section = -1;
		n1 = strlen(section);
		if (section[n1-1] == ':')
		    section[--n1] = '\0';
		for (mp = section_strs; mp < section_strs+M_SECTION_CNT; mp++)
		    if (!strcasecmp(section, mp->token)) {
			r_section = mp->val;
			break;
		    }
		if (r_section == -1) {
		    fprintf(stderr, "incorrect section name: %s\n", section);
		    exit (1);
		}
		if (r_section == S_ZONE) {
		    fprintf(stderr, "section ZONE not permitted\n");
		    exit (1);
		}
		/* read operation code */
		if (!getword_str(opcode, sizeof opcode, &startp, endp)) {
			fprintf(stderr, "failed to read operation code\n");
			exit (1);
		}
		r_opcode = -1;
		if (opcode[0] == '{') {
		    n1 = strlen(opcode);
		    for (i = 0; i < n1; i++)
			opcode[i] = opcode[i+1];
		    if (opcode[n1-2] == '}')
			opcode[n1-2] = '\0';
		}
		for (mp = opcode_strs; mp < opcode_strs+M_OPCODE_CNT; mp++) {
		    if (!strcasecmp(opcode, mp->token)) {
			r_opcode = mp->val;
			break;
		    }
		}
		if (r_opcode == -1) {
		    fprintf(stderr, "incorrect operation code: %s\n", opcode);
		    exit (1);
		}
		/* read owner's domain name */
		if (!getword_str(dnbuf, sizeof dnbuf, &startp, endp)) {
		    fprintf(stderr, "failed to read owner name\n");
		    exit (1);
		}
		r_dname = dnbuf;
		r_ttl = (r_opcode == ADD) ? -1 : 0;
		r_type = -1;
		r_class = C_IN; /* default to IN */
		r_size = 0;

		(void) getword_str(buf2, sizeof buf2, &startp, endp);

		if (isdigit(buf2[0])) { /* ttl */
		    r_ttl = strtoul(buf2, 0, 10);
		    if (errno == ERANGE && r_ttl == ULONG_MAX) {
			fprintf(stderr, "oversized ttl: %s\n", buf2);
			exit (1);
		    }
		    (void) getword_str(buf2, sizeof buf2, &startp, endp);
		}

		if (buf2[0]) { /* possibly class */
		    for (mp = class_strs; mp < class_strs+M_CLASS_CNT; mp++) {
			if (!strcasecmp(buf2, mp->token)) {
			    r_class = mp->val;
			    (void) getword_str(buf2, sizeof buf2, &startp, endp);
			    break;
			}
		    }
		}
		/*
		 * type and rdata field may or may not be required depending
		 * on the section and operation
		 */
		switch (r_section) {
		case S_PREREQ:
		    if (r_ttl) {
			fprintf(stderr, "nonzero ttl in prereq section: %lu\n",
				(u_long)r_ttl);
			r_ttl = 0;
		    }
		    switch (r_opcode) {
		    case NXDOMAIN:
		    case YXDOMAIN:
			if (buf2[0]) {
			    fprintf (stderr, "invalid field: %s, ignored\n",
				     buf2);
			    exit (1);
			}
			break;
		    case NXRRSET:
		    case YXRRSET:
			if (buf2[0])
			    for (mp = type_strs; mp < type_strs+M_TYPE_CNT; mp++)
				if (!strcasecmp(buf2, mp->token)) {
				    r_type = mp->val;
				    break;
				}
			if (r_type == -1) {
			    fprintf (stderr, "invalid type for RRset: %s\n",
				     buf2);
			    exit (1);
			}
			if (r_opcode == NXRRSET)
			    break;
			/*
			 * for RRset exists (value dependent) case,
			 * nonempty rdata field will be present.
			 * simply copy the whole string now and let
			 * res_update() interpret the various fields
			 * depending on type
			 */
			cp = startp;
			while (cp <= endp && isspace(*cp))
			    cp++;
			r_size = endp - cp + 1;
			break;
		    default:
			fprintf (stderr,
				 "unknown operation in prereq section\"%s\"\n",
				 opcode);
			exit (1);
		    }
		    break;
		case S_UPDATE:
		    switch (r_opcode) {
		    case DELETE:
			r_ttl = 0;
			r_type = T_ANY;
			/* read type, if specified */
			if (buf2[0])
			    for (mp = type_strs; mp < type_strs+M_TYPE_CNT; mp++)
				if (!strcasecmp(buf2, mp->token)) {
				    r_type = mp->val;
				    svstartp = startp;
				    (void) getword_str(buf2, sizeof buf2,
						       &startp, endp);
				    if (buf2[0]) /* unget preference */
					startp = svstartp;
				    break;
				}
			/* read rdata portion, if specified */
			cp = startp;
			while (cp <= endp && isspace(*cp))
			    cp++;
			r_size = endp - cp + 1;
			break;
		    case ADD:
			if (r_ttl == -1) {
			    fprintf (stderr,
		"ttl must be specified for record to be added: %s\n", buf);
			    exit (1);
			}
			/* read type */
			if (buf2[0])
			    for (mp = type_strs; mp < type_strs+M_TYPE_CNT; mp++)
				if (!strcasecmp(buf2, mp->token)) {
				    r_type = mp->val;
				    break;
				}
			if (r_type == -1) {
			    fprintf(stderr,
		"invalid type for record to be added: %s\n", buf2);
			    exit (1);
			}
			/* read rdata portion */
			cp = startp;
			while (cp < endp && isspace(*cp))
			    cp++;
			r_size = endp - cp + 1;
			if (r_size <= 0) {
			    fprintf(stderr,
		"nonempty rdata field needed to add the record at line %d\n",
				    lineno);
			    exit (1);
			}
			break;
		    default:
			fprintf(stderr,
		"unknown operation in update section \"%s\"\n", opcode);
			exit (1);
		    }
		    break;
		default:
		    fprintf(stderr,
			    "unknown section identifier \"%s\"\n", section);
		    exit (1);
		}

		if ( !(rrecp = res_mkupdrec(r_section, r_dname, r_class,
					    r_type, r_ttl)) ||
		     (r_size > 0 && !(rrecp->r_data = (u_char *)malloc(r_size))) ) {
			if (rrecp)
				res_freeupdrec(rrecp);
			fprintf(stderr, "saverrec error\n");
			exit (1);
		}
        if (stringtobin) {
             switch(r_opcode)  {
             case T_HINFO:
                  if (!getcharstring(buf,(char *)data,2,2,lineno))
                       exit(1);
                  cp = data;
                  break;
             case T_ISDN:
                  if (!getcharstring(buf,(char *)data,1,2,lineno))
                       exit(1);
                  cp = data;
                  break;
             case T_TXT:
                  if (!getcharstring(buf,(char *)data,1,0,lineno))
                       exit(1);
                  cp = data;
                  break;
             case T_X25:
                  if (!getcharstring(buf,(char *)data,1,1,lineno))
                       exit(1);
                  cp = data;
                  break;
             default:
		  break;
             }
        }
		rrecp->r_opcode = r_opcode;
		rrecp->r_size = r_size;
		(void) strncpy((char *)rrecp->r_data, cp, r_size);
		APPEND(listuprec, rrecp, r_link);
	    } else { /* end of an update packet */
		(void) res_ninit(&res);
		if (vc)
		    res.options |= RES_USEVC | RES_STAYOPEN;
		if (debug)
		    res.options |= RES_DEBUG;
		if (!EMPTY(listuprec)) {
			n = res_nupdate(&res, HEAD(listuprec),
					keyfile != NULL ? &key : NULL);
			if (n < 0)
				fprintf(stderr, "failed update packet\n");
			while (!EMPTY(listuprec)) {
				ns_updrec *tmprrecp = HEAD(listuprec);

				UNLINK(listuprec, tmprrecp, r_link);
				if (tmprrecp->r_size != 0)
					free((char *)tmprrecp->r_data);
				res_freeupdrec(tmprrecp);
			}
		}
	    }
	} /* for */
	return (0);
}

static void
usage() {
	fprintf(stderr, "Usage: %s [ -k keydir:keyname ] [-d] [-v] [file]\n",
		progname);
	exit(1);
}

/*
 * Get a whitespace delimited word from a string (not file)
 * into buf. modify the start pointer to point after the
 * word in the string.
 */
static int
getword_str(char *buf, int size, char **startpp, char *endp) {
        char *cp;
        int c;
 
        for (cp = buf; *startpp <= endp; ) {
                c = **startpp;
                if (isspace(c) || c == '\0') {
                        if (cp != buf) /* trailing whitespace */
                                break;
                        else { /* leading whitespace */
                                (*startpp)++;
                                continue;
                        }
                }
                (*startpp)++;
                if (cp >= buf+size-1)
                        break;
                *cp++ = (u_char)c;
        }
        *cp = '\0';
        return (cp != buf);
}

#define MAXCHARSTRING 255

static int
getcharstring(char *buf, char *data,
         int minfields, int maxfields, int lineno)
{
   int nfield = 0, n = 0, i;

   do {
        nfield++;
        i = 0;
        if (*buf == '"') {
             buf++;
             while(buf[i] && buf[i] != '"')
                  i++;
        } else {
             while(isspace(*buf))
                  i++;
        }
        if (i > MAXCHARSTRING) {
             fprintf(stderr,
               "%d: RDATA field %d too long",
               lineno, nfield);
             return(0);
        }
        if (n + i + 1 > MAXDATA) {
             fprintf(stderr,
               "%d: total RDATA too long", lineno);
             return(0);
        }
        data[n]=i;
        memmove(data + 1 + n, buf, i);
        buf += i + 1;
        n += i + 1;
        while(*buf && isspace(*buf))
             buf++;
   } while (nfield < maxfields && *buf);

   if (nfield < minfields) {
        fprintf(stderr,
             "%d: expected %d RDATA fields, only saw %d",
             lineno, minfields, nfield);
        return (0);
   }

   return (n);
}
