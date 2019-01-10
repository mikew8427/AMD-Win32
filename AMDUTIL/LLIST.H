//
// C++ Class for list list data elements
//
struct Mwlist
{
Mwlist *    MwPrev;      // Pointer to previous list entry 
Mwlist *    MwNext;      // Pointer to next lit entry       
char   *    MwData;      // Pointer to data item            
void   *    Mwptr;       // Pointer to ????  
};

// Class Definition for list list class
class Mwllist
{
private:
Mwlist  *   MwBaseF;                                      // Base first entry                 
Mwlist  *   MwBaseL;                                      // Base entry last entry             
unsigned int MwDlen;                                      // Lenth of each data item          
signed   int MwEmpty;                                     // Lenth of each data item  
       
public:
	Mwllist(unsigned int dsize=16);                       // Contructor to build a base entry 
	~Mwllist(void);                                       // Mr. Destructo
	int MwllAdd(char *data, void *extra=NULL);            // Add Data to list 
	int MwllAddEnt(Mwlist * ent);                         // Add entry to list 
	int MwllAddSorted(char *data, void *extra=NULL);      // Add entry to list sorted
  	int MwllRemove(Mwlist * ent);                         // Delete entry from list
  	int MwllDelete(Mwlist * ent);                         // Delete the storage for an entry 
  	int MwllInsertBefore(Mwlist * entn,Mwlist * ent);     // Insert entn before the one passed 
  	int MwllInsertAfter(Mwlist * entn,Mwlist * ent);      // Insert entn after the one passed 
	int MwClear(void);                                    // Clean up the list and storage     
	Mwlist * Mwllnew(void);                               // Create a New Entry
	Mwlist * MwllFindFirst(void);                         // Search list for match	
	Mwlist * MwllFindLast(void);                          // Get the last entry 
	Mwlist * MwllFindNext(Mwlist * ent);                  // Get the next in the list
	Mwlist * MwllFindPrev(Mwlist * ent);                  // Get the previous entry
	void MwllDump(void);                                  // Dump the entries of a list 
};
