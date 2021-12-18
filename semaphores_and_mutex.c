#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h> 

#define UNIT_CAPACITY 3     // Capacity of each unit
#define MAX_PEOPLE 60       // Total number of people to be tested
#define UNIT_NUM 8          // Number of units
#define HEALTHCARE_STAFF 8  // Total number of staffs in the hospital

int allDone = 0;                // Determining whether all people are tested
int lastUnitNo = UNIT_NUM - 1;  // To keep the last tested unit
int availableUnit;              // For the available test unit

int oneUnit[3] = {0, 0, 0};     //To keep the number of people in Unit-1
int twoUnit[3] = {0, 0, 0};     //To keep the number of people in Unit-2
int threeUnit[3] = {0, 0, 0};   //To keep the number of people in Unit-3
int fourUnit[3] = {0, 0, 0};    //To keep the number of people in Unit-4
int fiveUnit[3] = {0, 0, 0};    //To keep the number of people in Unit-5
int sixUnit[3] = {0, 0, 0};     //To keep the number of people in Unit-6
int sevenUnit[3] = {0, 0, 0};   //To keep the number of people in Unit-7
int eightUnit[3] = {0, 0, 0};   //To keep the number of people in Unit-8


void *people_f(void * number);  // Thread functionality created for people
void *staff_f(void * number);   // Thread functionality created for staffs

int controlForStarvation();                             // Finding the available unit and preventing the same unit in succession
void announcementStaff(int unitNo, int peopleNum);      // Control of the number of people in the unit and providing staff announcements
void displayPeopleNumInUnit(int unitNo, int peopleNum); // Display the latest status and numbers of people within the unit
void deletePeopleInUnit(int unitNo);                    // Emptying the array of people's numbers when the test is over

sem_t waitRoomHospital;             // Binary semaphore created to create seats in the hospital's large waiting room
sem_t unitWaitRoomSeat[UNIT_NUM];   // Defining a countable semaphore for each unit
sem_t criticalMutexUnit;            // Defining a critical semaphore according to the availability of units
sem_t checkTest[UNIT_NUM];          // Locking the unit that started the test, not taking people there, 
                                    // and preventing people from entering the unit until all people are out at the end of the test

pthread_t people_tid[MAX_PEOPLE];       // Define threads for people
pthread_t staff_tid[HEALTHCARE_STAFF];  // Define threads for staffs

void main(){
    printf("\n\t\t** \tCOVID-19 TEST UNIT\t **\n");
    printf("\n➾ The number of people who came to be tested was initially determined as %d.\n",MAX_PEOPLE);
    printf("➾ A solution to the COVID-19 TEST problem.\n\n");
    
    int i;                              //For use in loops
    int waitHospital[MAX_PEOPLE];       // Defined to value incoming people
    int staffInUnit[HEALTHCARE_STAFF];  // Defined to value determine staff

    // Placing elements in array so that it will be used for semaphore and thread values
    for (i = 0; i < HEALTHCARE_STAFF; i++) {
        staffInUnit[i] = i;
    }

    for (i = 0; i < MAX_PEOPLE; i++) {
        waitHospital[i] = i;
    }

    // Creating semaphores with initial values
    for(i = 0; i < UNIT_NUM; i++) {
        sem_init(&checkTest[i], 0, 1); // It was created as a binary semaphore because 
    }                                  // if a unit is being tested, it will not accept people until the test is completed.

    sem_init(&waitRoomHospital, 0, MAX_PEOPLE); // The hospital's large waiting room was initially described as a countable semaphore. 
                                                // Because every incoming person can go to the waiting room and be tested behind him/her.
                                                // So there is no situation of exceeding capacity.
   
    sem_init(&criticalMutexUnit, 0, 1); // It is to be used in critical situations and to direct people to the hospital when there is an empty unit. 
                                        // Otherwise, people were kept in the hospital's large waiting room. 

    for(i = 0; i < UNIT_NUM; i++){
        sem_init(&unitWaitRoomSeat[i], 0, UNIT_CAPACITY); // The initial value is given as much as the unit capacity so that as many people can enter. 
    }                                                     // A different semaphore was created for each unit.

    for (i = 0; i < HEALTHCARE_STAFF; i++) {
        pthread_create(&staff_tid[i], NULL, staff_f, (void *)&staffInUnit[i]); // Creating thread for staff and setting their values to elements in arrays
    }                                                                          // Multiple staff threads created because there is more than one staff
    
    for (i = 0; i < MAX_PEOPLE; i++) {
        pthread_create(&people_tid[i], NULL, people_f, (void *)&waitHospital[i]); // Creating thread for people and setting their values to elements in arrays
                                                                                  // Multiple person thread created because there is more than one person
        usleep(300000);  // The time it takes for people to come to the hospital
    }

    allDone = 1;    // When 'allDone' equals one, it means that all the people who came in were tested and finished.

    for (i = 0; i < MAX_PEOPLE; i++) {
        pthread_join(people_tid[i],NULL);   // All people have been tested and gone home
    }

    for (i = 0; i < HEALTHCARE_STAFF; i++) {
        pthread_join(staff_tid[i],NULL);    // All the staff did the tests and went home
    }
    
    printf("\n\t🠖 Covid-19 test was made to all people. 🠔 \n\n");
}

//The sample I received support while preparing this code was the sleeping barber sample. I wrote the code based on it because I thought it was a little different but logically similar for me.

// The person comes to the hospital and goes to the waiting room. The semaphore(sem_wait(&waitRoomHospital)) value is decreased by one.
// The unit(availableUnit = controlForStarvation()) that is available is found and the person is placed in that unit. Starvation is prevented. The same units in a row cannot be operated.
// The semaphore(sem_wait(&unitWaitRoomSeat[availableUnit])) value of that unit is reduced by one to determine the number of people inside.
// The semaphore(sem_post(&waitRoomHospital)) value of the hospital waiting room increases so that people can come to its place.
// The function(announcementStaff(availableUnit,peopleNum)) shows the last state in the unit.
// The critical semaphore(sem_post(&criticalMutexUnit)) value is increased by one so that waiting people go to available units.
void *people_f(void *number){
    
    int peopleNum = *(int *)number + 1; 

    sem_wait(&waitRoomHospital); //One person enters the hospital and the value of the waiting room is reduced by one
    printf("( ͡° ʖ̯ ͡°) ➪ People-%d entered the hospital and is waiting for the covid-19 test.\n\n", peopleNum);
    sleep(2);

    availableUnit = controlForStarvation();     // Find the appropriate unit
    sem_wait(&unitWaitRoomSeat[availableUnit]); // The value of the waiting room of the unit is decreased by one so that it is understood that a person enters inside
    sem_post(&waitRoomHospital);                // The value of the hospital's waiting room is increased by one, which means one more person has been put to the test

    printf("( ͡° ͜ʖ ͡°) ⤑ People-%d goes to the Unit-%d to be a Covid-19 test. People-%d is filling out a health form.\n",peopleNum, availableUnit + 1, peopleNum);
    announcementStaff(availableUnit,peopleNum); // Announcement of the people in the unit and the number of vacancies available
    sem_post(&criticalMutexUnit);               // The critical semaphore is incremented so that any unit has empty space
}

// The critical semaphore(sem_wait(&criticalMutexUnit)) is locked so that more than one action is not taken, people go to units one by one.
// The unit(sem_getvalue(&unitWaitRoomSeat[staffNum-1],&semValue)) value is found. This value is the empty space in the unit.
// The number of people(peopleCountInUnit) in the unit is found and different operations are performed accordingly.
// If there is 0(peopleCountInUnit == 0) people in the unit, it means the unit is empty.
// If there is 1 or 2(peopleCountInUnit == 1 || peopleCountInUnit == 2) person in the unit, it means 1 or 2 person is needed to start the test. 
// The announcement(announcementStaff(staffNum-1, -1)) about this is made by the personnel and the latest status of the unit is shown.
// If there is 3(peopleCountInUnit == 3) person in the unit, it means the test will start. 
// The announcement(announcementStaff(staffNum-1, -1)) about this is made by the personnel and the latest status of the unit is shown.
// In order to start the test, the semaphore(sem_wait(&checkTest[staffNum-1])) value of the unit is locked by one decrease so that no people enter the unit during the test.
// After the test is done, the unit is emptied by increasing the semaphore(sem_post(&unitWaitRoomSeat[staffNum-1])) value of the unit so that three people can exit at the same time.
// Then, the unit is unlocked(sem_post(&checkTest[staffNum-1])) and if there are people waiting, they are taken into the unit.
void *staff_f(void *number){

    int staffNum = *(int *)number + 1;
    int i;
    int semValue;
    int peopleCountInUnit;

    while (!allDone){ // With this cycle, it is checked whether the test of all people is completed

        sem_wait(&criticalMutexUnit); // Critical semaphore is locked to avoid any confusion
                                      // If people come at the same time, it allows them to take them into individual units
        sem_getvalue(&unitWaitRoomSeat[staffNum-1],&semValue); // The number of empty places in the unit is the semaphore value
        peopleCountInUnit = UNIT_CAPACITY - semValue;          // The number of people left in the unit is found. 
        // According to the conditions, either people are recruited to the unit or the test process is carried out.

        if(peopleCountInUnit == 0){

            sleep(2);
            announcementStaff(staffNum-1, -1); // Announcement of the people in the unit and the number of vacancies available
            printf("\nUnit-%d is empty. Staff-%d ventilates the Unit-%d.\n", staffNum, staffNum, staffNum);
        }
        else if(peopleCountInUnit == 1 || peopleCountInUnit == 2){

            sleep(2);
            announcementStaff(staffNum-1, -1); // Announcement of the people in the unit and the number of vacancies available
        } 
        else if(peopleCountInUnit == 3) {

            sleep(2);
            announcementStaff(staffNum-1, -1); // Announcement of the people in the unit and the number of vacancies available
            sem_wait(&checkTest[staffNum-1]);  // There is no free space in the unit[staffNum-1] anymore and the unit is locked to prevent people from coming

            // The test is over and all 3 people inside are taken out
            sem_post(&unitWaitRoomSeat[staffNum-1]); 
            sem_post(&unitWaitRoomSeat[staffNum-1]); 
            sem_post(&unitWaitRoomSeat[staffNum-1]);  

            printf("All people in the Unit-%d were tested. People got out of the Unit-%d.\n\n", staffNum, staffNum);

            sem_post(&checkTest[staffNum-1]); // The unit is not unlocked until all the people inside the unit are exited. 
                                              // Therefore, the unit is unlocked as all people are exited in the unit.
        }     
    }
}

// To prevent continuous testing in the same unit
// The last tested unit is kept and there is one unit to be reduced and new tested
// If he is also busy, he goes to the next unit
// With the while loop, all units are checked until an empty unit is found
int controlForStarvation(){

    int value;
    sem_getvalue(&unitWaitRoomSeat[lastUnitNo], &value);

    if(value == 2 || value == 1){
        return lastUnitNo;
    }
    else if(value == 3){
        return lastUnitNo;
    }
    else {
        lastUnitNo = lastUnitNo - 1;
        if(lastUnitNo < 0){
            lastUnitNo = lastUnitNo + UNIT_NUM;
        }
        sem_getvalue(&unitWaitRoomSeat[lastUnitNo], &value);
        
        while(value != 3){
 
            lastUnitNo = lastUnitNo - 1;
            if(lastUnitNo < 0){
                lastUnitNo = lastUnitNo + UNIT_NUM;
            }
            sem_getvalue(&unitWaitRoomSeat[lastUnitNo], &value);
        }   
        return lastUnitNo;
    }    
}

// The number of vacancies in the unit is announced by the personnel. Staff announces the situation inside the unit
void announcementStaff(int unitNo, int peopleNum){

    int i, tempI;
    int tempValue;
    int countPeople;

    sem_getvalue(&unitWaitRoomSeat[unitNo], &tempValue);    //The semaphore value of the unit to be processed is checked
    countPeople = UNIT_CAPACITY - tempValue;                //There is how many vacancies are left in that unit

    if(countPeople == 1 || countPeople == 2){

        displayPeopleNumInUnit(unitNo, peopleNum);
        printf(" ➟ Staff-%d : The last %d people, let's start! Please, pay attention to your social distance and hygiene; use a mask.\n", unitNo + 1, tempValue);
    }
    else if(countPeople == 3){

        displayPeopleNumInUnit(unitNo, peopleNum);
        printf(" ➟ Staff-%d : Unit-%d is full and soon the people in the unit will make tested for Covid-19.\n", unitNo + 1, unitNo + 1);
    }
    else if(countPeople == 0){

        deletePeopleInUnit(unitNo);
        printf(" ➟ Unit-%d is empty. \n", unitNo + 1);
    }
    printf("\n");
}

// Unit number and people number are taken as parameters so that who is placed in which unit is determined
//Which unit will be placed with the conditions has the interval of that unit
//If there are people in the unit, the numbers of the people are shown and then the newly added person is shown and added to the array. 
//If the unit has vacancies, they are also shown as empty.
void displayPeopleNumInUnit(int unitNo, int peopleNum){

    int i, tempI;
    int tempValue;

    printf("Unit-%d ☞ ",unitNo + 1);

    if(unitNo == 0){
        for(i = 0; i < UNIT_CAPACITY; i++){
            if(oneUnit[i] == 0)
                break;
            else{
                printf("[%d]",oneUnit[i]);
            }
        }
        tempI = i;
        if(i!=3){
            if(peopleNum !=-1){
                oneUnit[i] = peopleNum;
                printf("[%d]",oneUnit[i]);
                tempI = i + 1;
            }
            for(i = tempI; i<UNIT_CAPACITY; i++){
                printf("[ ]");
            }
        }
    }
    else if(unitNo == 1){
        for(i = 0; i < UNIT_CAPACITY; i++){
            if(twoUnit[i] == 0)
                break;
            else{
                printf("[%d]",twoUnit[i]);
            }
        }
        tempI = i;
        if(i!=3){
            if(peopleNum != -1){
                twoUnit[i] = peopleNum;
                printf("[%d]",twoUnit[i]);
                tempI = i + 1;
            }
            for(i = tempI; i<UNIT_CAPACITY; i++){
                printf("[ ]");
            }
        }
    }
    else if(unitNo == 2){
        for(i = 0; i < UNIT_CAPACITY; i++){
            if(threeUnit[i] == 0)
                break;
            else{
                printf("[%d]",threeUnit[i]);
            }
        }
        tempI = i;
        if(i!=3){
            if(peopleNum != -1){
                threeUnit[i] = peopleNum;
                printf("[%d]",threeUnit[i]);
                tempI = i + 1;
            }
            for(i = tempI; i<UNIT_CAPACITY; i++){
                printf("[ ]");
            }
        }
    }
    else if(unitNo == 3){
        for(i = 0; i < UNIT_CAPACITY; i++){
            if(fourUnit[i] == 0)
                break;
            else{
                printf("[%d]",fourUnit[i]);
            }
        }
        tempI = i;
        if(i!=3){
            if(peopleNum != -1){
                fourUnit[i] = peopleNum;
                printf("[%d]",fourUnit[i]);
                tempI = i + 1;
            }
            for(i = tempI; i<UNIT_CAPACITY; i++){
                printf("[ ]");
            }
        }
    }
    else if(unitNo == 4){
        for(i = 0; i < UNIT_CAPACITY; i++){
            if(fiveUnit[i] == 0)
                break;
            else{
                printf("[%d]",fiveUnit[i]);
            }
        }
        tempI = i;
        if(i!=3){
            if(peopleNum != -1){
                fiveUnit[i] = peopleNum;
                printf("[%d]",fiveUnit[i]);
                tempI = i + 1;
            }
            for(i = tempI; i<UNIT_CAPACITY; i++){
                printf("[ ]");
            }
        }
    }
    else if(unitNo == 5){
        for(i = 0; i < UNIT_CAPACITY; i++){
            if(sixUnit[i] == 0)
                break;
            else{
                printf("[%d]",sixUnit[i]);
            }
        }
        tempI = i;
        if(i!=3){
            if(peopleNum != -1){
                sixUnit[i] = peopleNum;
                printf("[%d]",sixUnit[i]);
                tempI = i + 1;
            }
            for(i = tempI; i<UNIT_CAPACITY; i++){
                printf("[ ]");
            }
        }
    }
    else if(unitNo == 6){
        for(i = 0; i < UNIT_CAPACITY; i++){
            if(sevenUnit[i] == 0)
                break;
            else{
                printf("[%d]",sevenUnit[i]);
            }
        }
        tempI = i;
        if(i!=3){
            if(peopleNum != -1){
               sevenUnit[i] = peopleNum;
                printf("[%d]",sevenUnit[i]);
                tempI = i + 1;
            }
            for(i = tempI; i<UNIT_CAPACITY; i++){
                printf("[ ]");
            }
        }
    }
    else{
        for(i = 0; i < UNIT_CAPACITY; i++){
            if(eightUnit[i] == 0)
                break;
            else{
                printf("[%d]",eightUnit[i]);
            }
        }
        tempI = i; 
        if(i!=3){
            if( peopleNum != -1){
                eightUnit[i] = peopleNum;
                printf("[%d]",eightUnit[i]);
                tempI = i + 1;
            }
            for(i = tempI; i<UNIT_CAPACITY; i++){
                printf("[ ]");
            }
        }
    }
}

//For people to empty the units at the end of the test and to make changes in the array where people's numbers are kept
//The array to empty for the unit number taken as a parameter is determined with the conditions.
void deletePeopleInUnit(int unitNo){

    int i;

    printf("Unit-%d ☞ ",unitNo + 1);

    if(unitNo == 0){
        for(i = 0; i < UNIT_CAPACITY; i++){
            printf("[ ]");
            oneUnit[i] = 0;
            }
    }
    else if(unitNo == 1){
        for(i = 0; i < UNIT_CAPACITY; i++){
            printf("[ ]");
            twoUnit[i] = 0;
            }   
    }
    else if(unitNo == 2){
        for(i = 0; i < UNIT_CAPACITY; i++){
            printf("[ ]");
            threeUnit[i] = 0;
        }        
    }
    else if(unitNo == 3){
        for(i = 0; i < UNIT_CAPACITY; i++){
            printf("[ ]");
            fourUnit[i] = 0;
        } 
    }
    else if(unitNo == 4){
        for(i = 0; i < UNIT_CAPACITY; i++){
            printf("[ ]");
            fiveUnit[i] = 0;
        }
    }
    else if(unitNo == 5){
        for(i = 0; i < UNIT_CAPACITY; i++){
            printf("[ ]");
            sixUnit[i] = 0;
        }
    }
    else if(unitNo == 6){
        for(i = 0; i < UNIT_CAPACITY; i++){
            printf("[ ]");
            sevenUnit[i] = 0;
        }  
    }
    else{
        for(i = 0; i < UNIT_CAPACITY; i++){
            printf("[ ]");
            eightUnit[i] = 0;
        }     
    }
}