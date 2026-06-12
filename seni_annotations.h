#ifndef SENI_ANNOTATIONS_H
#define SENI_ANNOTATIONS_H

/* Migration-intent annotations. The diff recovers structure but not intent:
   a same-type remove + add in one struct is either a rename (data must
   move) or a true removal (data must die), and seni refuses to guess.
   These macros expand to nothing for the compiler; seni's parser reads them
   from the header text -- the header itself is the intent channel, because
   seni parses source, not debug info.

   Rename (data moves from the old field):
       int light_count SENI_WAS(num_lights);

   Deliberate removal (silences the rename question; NO trailing semicolon,
   the macro expands to nothing and c89 forbids a bare ';' in a struct body):
       typedef struct {
           float x, y;
           SENI_DROPPED(num_lights)
       } enemy;

   Annotations can stay in the header forever: SENI_WAS is consulted only
   when the field has no same-name match in the old layout, so once the
   rename has migrated it becomes inert history. */

#define SENI_WAS(old_name)
#define SENI_DROPPED(old_name)

#endif
