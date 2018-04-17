#include <stdio.h>
#include <assert.h>
#include<string.h>
#include <sys/time.h>
#include <pthread.h>

int NumCats,NumMice,Numbowls,cats_eats,mouse_eats;

#define NumBowls  2         

const int cat_wait=10;           
const int cat_eat=4;
const int mouse_wait=10;     
const int mouse_eat=2;


typedef struct FoodBowl
	 {
      int free_Bowls;             
      int cats_eating;            
      int mice_eating;            
      int cats_waiting;           
    enum {
        none_eating,
        cat_eating,
        mouse_eating
    } status[NumBowls];         
    pthread_mutex_t mutex;      
    pthread_cond_t free_cv;     
    pthread_cond_t cat_cv;      
} FoodBowl_t;


int NumCats,NumMice;

static const char *progname = "CAT AND MICE SYNCHRONIZATION PROBLEM";

static void Display(const char *name,const char *what,FoodBowl_t *FoodBowl, int my_FoodBowl)
{
    int i;
     printf("\n[");
    for (i = 0; i < NumBowls; i++) {
        if (i) 
	     printf(":");
        switch (FoodBowl->status[i]) {
        case none_eating:
            printf("-");
            break;
        case cat_eating:
            printf("cat");
            break;
        case mouse_eating:
            printf("mice");
            break;
        }
    }
    printf("]  %s %s eating from FoodBowl %d\n", name, what, my_FoodBowl);
}



static void*cat(void *arg)
{
    FoodBowl_t *FoodBowl = (FoodBowl_t *) arg;
    int n = cats_eats;
    int my_FoodBowl = -1;
    int i;

    for (n = cats_eats; n > 0; n--) {

        pthread_mutex_lock(&FoodBowl->mutex);
    
        pthread_cond_broadcast(&FoodBowl->cat_cv);
        
		FoodBowl->cats_waiting++;
        
	while (FoodBowl->free_Bowls <= 0 || FoodBowl->mice_eating > 0) 
	{
          pthread_cond_wait(&FoodBowl->free_cv, &FoodBowl->mutex);
        }
        FoodBowl->cats_waiting--;

        assert(FoodBowl->free_Bowls > 0);
        FoodBowl->free_Bowls--;
        assert(FoodBowl->cats_eating < NumCats);
        FoodBowl->cats_eating++;
        
        
        for (i = 0; i < NumBowls && FoodBowl->status[i] != none_eating; i++) ;
         my_FoodBowl = i;
        
		assert(FoodBowl->status[my_FoodBowl] == none_eating);
        FoodBowl->status[my_FoodBowl] = cat_eating;
        Display("cat", "started", FoodBowl, my_FoodBowl);
        pthread_mutex_unlock(&FoodBowl->mutex);

        sleep(cat_eat);
        
        pthread_mutex_lock(&FoodBowl->mutex);
        assert(FoodBowl->free_Bowls < NumBowls);
        FoodBowl->free_Bowls++;
        assert(FoodBowl->cats_eating > 0);
        FoodBowl->cats_eating--;
        FoodBowl->status[my_FoodBowl] = none_eating;

      
        pthread_cond_broadcast(&FoodBowl->free_cv);
        Display("cat","finished", FoodBowl, my_FoodBowl);
        pthread_mutex_unlock(&FoodBowl->mutex);

        sleep(rand() % cat_wait);
    }

    return NULL;
}


static void*mouse(void *arg)
{
    FoodBowl_t *FoodBowl = (FoodBowl_t *) arg;
    int n = mouse_eats;
    struct timespec t1;
    struct timeval t2;
    int my_FoodBowl;
    int i;

    for (n = mouse_eats; n > 0; n--)
	 {
        pthread_mutex_lock(&FoodBowl->mutex);
        
        while (FoodBowl->free_Bowls <= 0 || FoodBowl->cats_eating > 0 || FoodBowl->cats_waiting > 0)
		 {
            pthread_cond_wait(&FoodBowl->free_cv, &FoodBowl->mutex);
         }

        assert(FoodBowl->free_Bowls > 0);
        FoodBowl->free_Bowls--;
        assert(FoodBowl->cats_eating == 0);
        assert(FoodBowl->mice_eating < NumMice);
        FoodBowl->mice_eating++;

        for (i = 0; i < NumBowls && FoodBowl->status[i] != none_eating; i++) ;
        my_FoodBowl = i;
        
        assert(FoodBowl->status[my_FoodBowl] == none_eating);
        FoodBowl->status[my_FoodBowl] = mouse_eating;
        
		Display("mouse","started", FoodBowl, my_FoodBowl);
        
		pthread_mutex_unlock(&FoodBowl->mutex);
        
     
        t1.tv_sec  = t2.tv_sec;
        t1.tv_nsec = t2.tv_usec * 1000;
        t1.tv_sec += mouse_eat;
        pthread_mutex_lock(&FoodBowl->mutex);
        pthread_cond_timedwait(&FoodBowl->cat_cv, &FoodBowl->mutex, &t1);
        pthread_mutex_unlock(&FoodBowl->mutex);
        
        pthread_mutex_lock(&FoodBowl->mutex);
        assert(FoodBowl->free_Bowls < NumBowls);
        FoodBowl->free_Bowls++;
        assert(FoodBowl->cats_eating == 0);
        assert(FoodBowl->mice_eating > 0);
        FoodBowl->mice_eating--;
        FoodBowl->status[my_FoodBowl]=none_eating;

        
        pthread_cond_broadcast(&FoodBowl->free_cv);
        Display("mouse","finished", FoodBowl, my_FoodBowl);
        pthread_mutex_unlock(&FoodBowl->mutex);
        
        /* sleep to avoid Starvation */
        sleep(rand() % mouse_wait);
    }

    return NULL;
}


int main(int argc, char *argv[])
{
    int i, err;
    FoodBowl_t _FoodBowl, *FoodBowl;
	
	printf("Enter the number of cats: ");
	scanf("%d",&NumCats);

	printf("Enter the number of Mice: ");
	scanf("%d",&NumMice);

	printf(" how many times a cat wants to eat: ");
	scanf("%d",&cats_eats);

	printf(" how many times a mice wants to eat: ");
	scanf("%d",&mouse_eats);


    pthread_t cats[NumCats];
    pthread_t mice[NumMice];

    srand(time(NULL)); 
    FoodBowl = &_FoodBowl;
    memset(FoodBowl, 0, sizeof(FoodBowl_t));
    FoodBowl->free_Bowls = NumBowls;
    pthread_mutex_init(&FoodBowl->mutex, NULL);
    pthread_cond_init(&FoodBowl->free_cv, NULL);
    pthread_cond_init(&FoodBowl->cat_cv, NULL);
    
     printf("\n !!!Cat And Mice Synchronization for  Eating from Bowls!!!\n\n");
     printf("[B1,B2] BOWLS OCCUPIED BY ");
    
    for (i = 0; i < NumCats; i++) {
        err = pthread_create(&cats[i], NULL, cat, FoodBowl);
        if (err != 0) {
            printf("unable to create cat thread");
        }
    }

    for (i = 0; i < NumMice; i++) {
        err = pthread_create(&mice[i], NULL, mouse, FoodBowl);
         if (err != 0) 
		{
            printf(" unable to create mouse thread");
        } 
    }


    for (i = 0; i < NumCats; i++) 
	{
         pthread_join(cats[i], NULL);
    }
    for (i = 0; i < NumMice; i++)
	 {
         pthread_join(mice[i], NULL);
    }
    
    pthread_mutex_destroy(&FoodBowl->mutex);
    pthread_cond_destroy(&FoodBowl->free_cv);
    pthread_cond_destroy(&FoodBowl->cat_cv);
    
    return 0;
}
