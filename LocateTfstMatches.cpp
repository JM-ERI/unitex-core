 /*
  * Unitex
  *
  * Copyright (C) 2001-2009 Universit� Paris-Est Marne-la-Vall�e <unitex@univ-mlv.fr>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Lesser General Public
  * License as published by the Free Software Foundation; either
  * version 2.1 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Lesser General Public License for more details.
  *
  * You should have received a copy of the GNU Lesser General Public
  * License along with this library; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
  *
  */

#include "LocateTfstMatches.h"
#include "Error.h"
#include <stdlib.h>
#include "Tfst.h"


/**
 * Allocates, initializes and returns a new tfst_match.
 */
struct tfst_match* new_tfst_match(int source_state_text,
                                  int dest_state_text,
                                  Transition* fst2_transition,
                                  int pos_kr,
                                  int text_tag_number) {
struct tfst_match* match=(struct tfst_match*)malloc(sizeof(struct tfst_match));
if (match==NULL) {
   fatal_alloc_error("new_tfst_match");
}
match->source_state_text=source_state_text;
match->dest_state_text=dest_state_text;
match->fst2_transition=fst2_transition;
match->pos_kr=pos_kr;
match->text_tag_numbers=sorted_insert(text_tag_number,NULL);
match->next=NULL;
match->pointed_by=0;
return match;
}


/**
 * Inserts a tfst_match corresponding to the given information, if not already present.
 */
struct tfst_match* insert_in_tfst_matches(struct tfst_match* list,
                                          int source_state_text,
                                          int dest_state_text,
                                          Transition* fst2_transition,
                                          int pos_kr,
                                          int text_tag_number) {
if (list==NULL) {
   return new_tfst_match(source_state_text,dest_state_text,
         fst2_transition,pos_kr,text_tag_number);
}
if (list->source_state_text==source_state_text
    && list->dest_state_text==dest_state_text
    && list->fst2_transition==fst2_transition
    && list->pos_kr==pos_kr) {
    list->text_tag_numbers=sorted_insert(text_tag_number,list->text_tag_numbers);
    return list;
}
list->next=insert_in_tfst_matches(list->next,
                         source_state_text,dest_state_text,
                         fst2_transition,pos_kr,text_tag_number);
return list;
}


/**
 * Frees the given tfst_match.
 */
void free_tfst_match(struct tfst_match* match) {
if (match==NULL) {
   fatal_error("NULL error in free_tfst_match");
}
/* We MUST NOT free 'fst2_transition' since it is just a copy of an actual transition
 * in the grammar */
free_list_int(match->text_tag_numbers);
free(match);
}


/**
 * Allocates, initializes and returns a tfst_match_list.
 */
struct tfst_match_list* new_tfst_match_list() {
struct tfst_match_list* l=(struct tfst_match_list*)malloc(sizeof(struct tfst_match_list*));
if (l==NULL) {
   fatal_alloc_error("new_tfst_match_list");
}
l->match=NULL;
l->next=NULL;
return l;
}


/**
 * Adds a tfst_match to the given list, increasing its reference counter.
 */
struct tfst_match_list* add_match_in_list(struct tfst_match_list* list,
                                          struct tfst_match* match) {
struct tfst_match_list* l=new_tfst_match_list();
l->match=match;
l->next=list;
(match->pointed_by)++;
return l;
}


/**
 * This function takes a list of match elements and free the first elements
 * until they have a non null pointed_by field. In other words, this function
 * releases all the first elements that are not involved anymore in any match.
 * The limit parameter refers to a stop element. In fact, when we want to
 * clean a list of match elements associated to a subgraph call, we do not want
 * to remove the elements that do not depend on this subgraph call.
 */
void clean_tfst_match_list(struct tfst_match* list,struct tfst_match* limit) {
struct tfst_match* tmp;
while (list!=NULL && list!=limit && list->pointed_by==0) {
   /* We have an element that we can free */
   if (list->next!=NULL) {
      /* If there is a next element, then we say
       * that it is now pointed by one element less */
      (list->next->pointed_by)--;
   }
   tmp=list->next;
   free_tfst_match(list);
   list=tmp;
}
}


/**
 * This function clones the given element. Note that we increase the 'pointed_by' field
 * of the tfst_match.
 */
struct tfst_simple_match_list* new_tfst_simple_match_list(struct tfst_simple_match_list* e,
		                                                  struct tfst_simple_match_list* next) {
struct tfst_simple_match_list* m=(struct tfst_simple_match_list*)malloc(sizeof(struct tfst_simple_match_list));
if (m==NULL) {
	fatal_alloc_error("new_tfst_simple_match_list");
}
memcpy(m,e,sizeof(struct tfst_simple_match_list));
if (m->match!=NULL) {
	(m->match->pointed_by)++;
}
m->next=next;
return m;
}


/**
 * Frees the given element. Note that we decrease the 'pointed_by' field
 * of the tfst_match.
 */
void free_tfst_simple_match_list(struct tfst_simple_match_list* m) {
if (m==NULL) {
	return;
}
if (m->match!=NULL) {
	(m->match->pointed_by)--;
}
free(m->output);
free(m);
}


/**
 * This function looks for the first element that is not text independent
 */
struct tfst_match* find_first_text_dependent_tag(struct tfst_match* list) {
while (list!=NULL) {
   struct list_int* l=list->text_tag_numbers;
   while (l!=NULL) {
      if (l->n!=-1) {
         /* If there is at least one text dependent tag, 'list' is the one */
         return list;
      }
      l=l->next;
   }
   list=list->next;
}
/* If the list has no text dependent tag, we fail */
return NULL;
}


/**
 * This function looks for the last element that is not text independent
 */
struct tfst_match* find_last_text_dependent_tag(struct tfst_match* m) {
if (m==NULL) {
   return NULL;
}
struct tfst_match* candidate=find_last_text_dependent_tag(m->next);
if (candidate!=NULL) {
   /* If there is a candidate, we return it */
   return candidate;
}
/* If there is no candidate, maybe 'm' could be the one */
struct list_int* list=m->text_tag_numbers;
while (list!=NULL) {
   if (list->n!=-1) {
      /* If there is at least one text dependent tag, 'm' is the one */
      return m;
   }
   list=list->next;
}
/* If 'm' has no text dependent tag, we fail */
return NULL;
}


/**
 * This function extracts simple information from a complex tfst_match list.
 */
static void fill_element(struct locate_tfst_infos* infos,struct tfst_simple_match_list* e,struct tfst_match* m) {
/* First, we compute the end offsets with the first element of the list
 * (remember: a match list is reversed) */
e->end_pos_in_token=-1;
e->end_pos_in_char=-1;
e->end_pos_in_letter=-1;
struct tfst_match* first_text_dependent_tag=find_first_text_dependent_tag(m);
if (first_text_dependent_tag==NULL) {
   /* If the whole match is text independent, we fail */
   e->end_pos_in_token=-1;
   return;
}
struct list_int* tags=first_text_dependent_tag->text_tag_numbers;
while (tags!=NULL) {
   if (tags->n!=-1) {
      /* We only consider tags that are not text independent */
	   TfstTag* t=(TfstTag*)(infos->tfst->tags->tab[tags->n]);
	   if (e->end_pos_in_token==-1 || e->end_pos_in_token<t->end_pos_token
	       || e->end_pos_in_char<t->end_pos_char
	       || e->end_pos_in_letter<t->end_pos_letter) {
		   e->end_pos_in_token=t->end_pos_token;
		   e->end_pos_in_char=t->end_pos_char;
		   e->end_pos_in_letter=t->end_pos_letter;
	   }
   }
	tags=tags->next;
}

/* Then, we locate the last element in order to get information about the start offset */
struct tfst_match* last_text_dependent_tag=find_last_text_dependent_tag(m);
if (last_text_dependent_tag==NULL) {
   /* If the whole match is text independent, we fail */
   e->end_pos_in_token=-1;
   return;
}
e->start_pos_in_token=-1;
e->start_pos_in_char=-1;
e->start_pos_in_letter=-1;
tags=last_text_dependent_tag->text_tag_numbers;
while (tags!=NULL) {
   if (tags->n!=-1) {
      /* We only consider tags that are not text independent */
	   TfstTag* t=(TfstTag*)(infos->tfst->tags->tab[tags->n]);
	   if (e->start_pos_in_token==-1 || e->start_pos_in_token>t->start_pos_token
	       || e->start_pos_in_char>t->start_pos_char
	       || e->start_pos_in_letter>t->start_pos_letter) {
		   e->start_pos_in_token=t->start_pos_token;
		   e->start_pos_in_char=t->start_pos_char;
		   e->start_pos_in_letter=t->start_pos_letter;
	   }
   }
	tags=tags->next;
}
/* Finally, we adjust offsets with the base offset of the current sentence */
e->start_pos_in_token+=infos->tfst->offset_in_tokens;
e->end_pos_in_token+=infos->tfst->offset_in_tokens;
e->output=NULL;
e->match=NULL;
e->next=NULL;
}


/**
 * Returns 1 if a is longer than b; 0 otherwise.
 */
int is_longer_match(struct tfst_simple_match_list* a,struct tfst_simple_match_list* b) {
if (a->start_pos_in_token>b->start_pos_in_token) return 0;
if (a->start_pos_in_token==b->start_pos_in_token) {
   if (a->start_pos_in_char>b->start_pos_in_char) return 0;
   if (a->start_pos_in_char==b->start_pos_in_char) {
      if (a->start_pos_in_letter>b->start_pos_in_letter) return 0;
   }
}
if (a->end_pos_in_token<b->end_pos_in_token) return 0;
if (a->end_pos_in_token==b->end_pos_in_token) {
   if (a->end_pos_in_char<b->end_pos_in_char) return 0;
   if (a->end_pos_in_char==b->end_pos_in_char) {
      if (a->end_pos_in_letter<b->end_pos_in_letter) return 0;
   }
}
return 1;
}


/**
 * For given actual match:
 * 1. checks if there are "longer" matches in the list and eliminates them
 * 2. if there is no "shorter" match in the list, adds the actual match to the list
 *
 * # added for support of ambiguous transducers:
 * 3. matches with same range but different output are also accepted
 *
 * 'dont_add_match' is set to 1 if any shorter match is found, i.e. if we
 * won't have to insert the new match into the list; to 0 otherwise.
 * NOTE: 'dont_add_match' is supposed to be initialized at 0 before this
 *       funtion is called.
 */
struct tfst_simple_match_list* eliminate_longer_matches(struct tfst_simple_match_list* ptr,
                                            struct tfst_simple_match_list* e,
                                            int *dont_add_match,
                                            struct locate_tfst_infos* p) {
int start=e->start_pos_in_token;
int end=e->end_pos_in_token;
int start_in_char=e->start_pos_in_char;
int end_in_char=e->end_pos_in_char;
int start_in_letter=e->start_pos_in_letter;
int end_in_letter=e->end_pos_in_letter;
struct tfst_simple_match_list* l;
if (ptr==NULL) return NULL;
if (p->ambiguous_output_policy==ALLOW_AMBIGUOUS_OUTPUTS
    && ptr->start_pos_in_token==start && ptr->end_pos_in_token==end
    && ptr->start_pos_in_char==start_in_char && ptr->end_pos_in_char==end_in_char
    && ptr->start_pos_in_letter==start_in_letter && ptr->end_pos_in_letter==end_in_letter
    && u_strcmp(ptr->output,e->output)) {
    /* In the case of ambiguous transductions producing different outputs,
     * we accept matches with same range */
   ptr->next=eliminate_longer_matches(ptr->next,e,dont_add_match,p);
   return ptr;
}
if (is_longer_match(ptr,e)) {
   /* If the new match is shorter (or of equal length) than the current one
    * in the list, we replace the match in the list by the new one */
   if (*dont_add_match) {
      /* If we have already noticed that the match mustn't be added
       * to the list, we delete the current list element */
      l=ptr->next;
      free_tfst_simple_match_list(ptr);
      return eliminate_longer_matches(l,e,dont_add_match,p);
    }
    /* If the new match is shorter than the current one in the list, then we
     * update the current one with the value of the new match. */
    ptr->start_pos_in_token=start;
    ptr->end_pos_in_token=end;
    ptr->start_pos_in_char=e->start_pos_in_char;
    ptr->end_pos_in_char=e->end_pos_in_char;
    ptr->start_pos_in_letter=e->start_pos_in_letter;
    ptr->end_pos_in_letter=e->end_pos_in_letter;
    if (ptr->match!=NULL) {
    	(ptr->match->pointed_by)--;
    }
    /* e->match->pointed_by does not need to be changed */
    ptr->match=e->match;
    if (ptr->output!=NULL) free(ptr->output);
    ptr->output=u_strdup(e->output);
    /* We note that the match does not need anymore to be added */
    (*dont_add_match)=1;
    ptr->next=eliminate_longer_matches(ptr->next,e,dont_add_match,p);
    return ptr;
}
if (is_longer_match(e,ptr)) {
   /* The new match is longer than the one in list => we
    * skip the new match */
   (*dont_add_match)=1;
   return ptr;
}
/* If we have disjunct ranges or overlapping ranges without inclusion,
 * we examine recursively the rest of the list */
ptr->next=eliminate_longer_matches(ptr->next,e,dont_add_match,p);
return ptr;
}


/**
 * Returns 1 if a ends strictly after b.
 */
int match_end_after(struct tfst_simple_match_list* a,struct tfst_simple_match_list* b) {
if (a->end_pos_in_token<b->end_pos_in_token) return 0;
if (a->end_pos_in_token>b->end_pos_in_token) return 1;
/* Same end positions in tokens */
if (a->end_pos_in_char<b->end_pos_in_char) return 0;
if (a->end_pos_in_char>b->end_pos_in_char) return 1;
/* Same end positions in chars */
if (a->end_pos_in_letter<=b->end_pos_in_letter) return 0;
return 1;
}


/**
 * Returns 1 if a begins exactly at the same position that b.
 */
int same_start_positions(struct tfst_simple_match_list* a,struct tfst_simple_match_list* b) {
return a->start_pos_in_token==b->start_pos_in_token
    && a->start_pos_in_char==b->start_pos_in_char
    && a->start_pos_in_letter==b->start_pos_in_letter;
}


/**
 * Returns 1 if a ends exactly at the same position that b.
 */
int same_end_positions(struct tfst_simple_match_list* a,struct tfst_simple_match_list* b) {
return a->end_pos_in_token==b->end_pos_in_token
    && a->end_pos_in_char==b->end_pos_in_char
    && a->end_pos_in_letter==b->end_pos_in_letter;
}


/**
 * Returns 1 if a and b have the same bounds.
 */
int same_positions(struct tfst_simple_match_list* a,struct tfst_simple_match_list* b) {
return same_start_positions(a,b) && same_end_positions(a,b); 
}


/**
 * Adds a match to the global list of matches. The function takes into
 * account the match policy. For instance, we don't take [2;3] into account
 * if we are in longest match mode and if we already have [2;5].
 *
 * This function is derived from the 'add_match' one in 'Matches.cpp'.
 *
 * IMPORTANT: 'e' is to be copied if the corresponding match must be added to the list */
void add_element_to_list(struct locate_tfst_infos* p,struct tfst_simple_match_list* e) {
int start=e->start_pos_in_token;
int end=e->end_pos_in_token;
struct tfst_simple_match_list* l;
if (p->matches==NULL) {
   /* If the match list was empty, we always can put the match in the list */
   p->matches=new_tfst_simple_match_list(e,NULL);
   return;
}
switch (p->match_policy) {
   case LONGEST_MATCHES:
      /* We put new matches at the beginning of the list */
      if (match_end_after(e,p->matches)) {
         /* In longest match mode, we only consider matches ending
          * later. Moreover, we allow just one match from a given
          * start position, except if ambiguous outputs are allowed. */
         if (same_start_positions(p->matches,e)) {
            /* We overwrite matches starting at same position but ending earlier.
             * We do this by deleting the actual head element of the list
             * and calling the function recursively.
             * This works also for different outputs from ambiguous transducers,
             * i.e. we may delete more than one match in the list. */
            l=p->matches;
            p->matches=p->matches->next;
            free_tfst_simple_match_list(l);
            add_element_to_list(p,e);
            return;
         }
         /* We allow add shorter matches but with other start position */
         p->matches=new_tfst_simple_match_list(e,p->matches);
         return;
      }
      /* If we have the same start and the same end, we consider the
       * new match only if ambiguous outputs are allowed */
      if (p->ambiguous_output_policy==ALLOW_AMBIGUOUS_OUTPUTS
         && same_positions(p->matches,e)
         && u_strcmp(p->matches->output,e->output)) {
         /* Because matches with same range and same output may not come
          * one after another, we have to look if a match with same output
          * already exists */
         l=p->matches;
         while (l!=NULL && u_strcmp(l->output,e->output)) {
            l=l->next;
         }
         if (l==NULL) {
            p->matches=new_tfst_simple_match_list(e,p->matches);
         }
      }
      break;

   case ALL_MATCHES:
      /* We put new matches at the beginning of the list */
      l=p->matches;
      /* We unify identical matches, i.e. matches with same range (start and end),
       * taking the output into account if ambiguous outputs are allowed. */
      while (l!=NULL
             && !(same_positions(l,e)
                  && (p->ambiguous_output_policy!=ALLOW_AMBIGUOUS_OUTPUTS
                      || u_strcmp(l->output,e->output)))) {
         l=l->next;
      }
      if (l==NULL) {
         p->matches=new_tfst_simple_match_list(e,p->matches);
      }
      break;

   case SHORTEST_MATCHES:
      /* We put the new match at the beginning of the list, but before, we
       * test if the match we want to add may not be discarded because of
       * a shortest match that would already be in the list. By the way,
       * we eliminate matches that are longer than this one, if any. */
      int dont_add_match=0;
      p->matches=eliminate_longer_matches(p->matches,e,&dont_add_match,p);
      if (!dont_add_match) {
         p->matches=new_tfst_simple_match_list(e,p->matches);
      }
      break;
   }
}



/**
 * This function takes a tfst_match list that represents a match. It turns it into
 * a tfst_simple_match_list element and inserts it into to the main tfst_simple_match_list,
 * according to the match filtering rules.
 */
void add_tfst_match(struct locate_tfst_infos* infos,struct tfst_match* m) {
struct tfst_simple_match_list element;
fill_element(infos,&element,m);
if (element.end_pos_in_token==-1) {
   /* If the match was in fact completely text independent, then we reject it */
   return;
}
//error("match from token %d to %d\n",element.start_pos_in_token,element.end_pos_in_token);
add_element_to_list(infos,&element);
}


/**
 * This function saves the current match list in the concordance index file.
 * It is derived from the 'save_matches' from 'Matches.cpp'. At the opposite of
 * 'save_matches', this function is not parameterized by the current position in
 * the text. We assume that this function is called once per sentence automaton, after
 * all matches have been computed.
 */
void save_tfst_matches(struct locate_tfst_infos* p) {
struct tfst_simple_match_list* l=p->matches;
struct tfst_simple_match_list* ptr;
if (p->number_of_matches==p->search_limit) {
	/* If we have reached the limit, then we must free all the remaining matches */
	while (l!=NULL) {
		ptr=l;
		l=l->next;
		free_tfst_simple_match_list(ptr);
	}
	p->matches=NULL;
	return;
}
U_FILE* f=p->output;
if (l==NULL) return;
u_fprintf(f,"%d.%d.%d %d.%d.%d",l->start_pos_in_token,l->start_pos_in_char,
      l->start_pos_in_letter,l->end_pos_in_token,
      l->end_pos_in_char,l->end_pos_in_letter);
if (l->output!=NULL) {
	/* If there is an output */
	u_fprintf(f," %S",l->output);
}
u_fprintf(f,"\n");
if (p->ambiguous_output_policy==ALLOW_AMBIGUOUS_OUTPUTS) {
   (p->number_of_outputs)++;
   if (!(p->start_position_last_printed_match == l->start_pos_in_token
	   && p->end_position_last_printed_match == l->end_pos_in_token)) {
	   (p->number_of_matches)++;
   }
} else {
	/* If we don't allow ambiguous outputs, we count the matches */
	(p->number_of_matches)++;
}
p->start_position_last_printed_match=l->start_pos_in_token;
p->end_position_last_printed_match=l->end_pos_in_token;
if (p->number_of_matches==p->search_limit) {
	/* If we have reached the search limitation, we free the remaining
	 * matches and return */
	while (l!=NULL) {
		ptr=l;
		l=l->next;
		free_tfst_simple_match_list(ptr);
	}
	p->matches=NULL;
	return;
}
ptr=l->next;
free_tfst_simple_match_list(l);
p->matches=ptr;
save_tfst_matches(p);
return;
}
