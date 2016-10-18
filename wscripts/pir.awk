# File wscripts/pir.awk, postprocessing commands used by blast_search script
# $Id: pir.awk,v 1.1 1997-03-28 12:59:53 mieg Exp $

BEGIN { state = 0 ;}
/^ENTRY/ {seq = "PIR:"$2 ; printf("\nProtein %s\nDatabase PIR %s\n",seq,$2); next;}
/^TITLE/ {printf("Title \"%s\n", substr($0,6)) ; next ;}
/^\/\/\// {state = 0 ; printf("\n") ; next; }

/^SEQUENCE/ {printf ("Peptide %s\n\n",seq) ; state = 2 ; next ;}
{ if (state == 2) { printf("Peptide %s\n", seq) ; state = 3 ; next ; }
  if (state == 3) 
    { pp = $0 ; gsub(/[0-9]/,"",pp) ; gsub(/ /,"",pp) ; gsub(/\//,"",pp) ;
      printf("%s\n", pp) ;
    }
}
