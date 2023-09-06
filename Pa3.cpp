/*Programing assignment 3
  Name: Ngoc Phuc An Nguyen
  Code with the help of Geeksforgeeks
  link: https://www.geeksforgeeks.org/huffman-coding-greedy-algo-3/
  I divide assignment into three part:
  First: create Huffman tree
  Second: passing data of huffman tree to pthread
  Third: pthread for original message
*/

#include <iostream>
#include <string>
#include <cstring>
#include <pthread.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <bits/stdc++.h>

// A Huffman tree node
struct MinHeapNode
{

    // One of the input characters
    std::string data;

    // Frequency of the character
    unsigned freq;

    // sort between sum of 2 node
    int sorting_int;

    // Left and right child
    MinHeapNode *left, *right;

    MinHeapNode(std::string data, unsigned freq, int sorting_int)

    {

        left = right = NULL;
        this->data = data;
        this->freq = freq;
        this->sorting_int = sorting_int;
    }
};

// struct to store data of every node
struct nodeData
{
    std::vector<std::string> character;
    std::vector<std::string> binary_code;
    std::vector<int> frequency;
};

// struct to compare between 2 node (help of geeksforgeeks)
struct compare
{
    bool operator()(MinHeapNode *l, MinHeapNode *r)
    {
        if (l->freq == r->freq)
        {
            if (l->data.length() == 1 && r->data.length() == 1)
            {
                return (l->data[0] > r->data[0]);
            }
            else if (l->data.length() != 1 && r->data.length() != 1)
            {
                return (l->sorting_int < r->sorting_int);
            }
            else
            {
                return (l->data != "sum");
            }
        }
        else
        {
            return (l->freq > r->freq);
        }
    }
};

// function to decide original message length
int original_message_length(std::vector<int> a, int *b)
{
    for (int i = 0; i < a.size(); i++)
    {
        *b += a[i];
    }
    return 0;
};

// struct for pthread, including line of string from compressed file, 2 vector of
// character and it binary code after huffman tree, a final string.
struct dataForPthread
{
    std::string compressed_line;
    std::vector<std::string> pthread_character;
    std::vector<std::string> pthread_binary_code;
    std::vector<int> pthread_frequency;
    std::string *originalMessage;
    // properties for mutex
    pthread_mutex_t *bsem;
    pthread_mutex_t *parentThread;
    pthread_cond_t *waitTurn;
    // mutex condition
    int *threadNumber;
    int *turn;
};

// store data of a node,  combine with struct nodeData,
// to provide information of every node (geeksforgeeks)
void printCodes(struct MinHeapNode *root, std::string str, struct nodeData *name)
{

    if (!root)
        return;

    if (root->data != "sum")
    {
        name->character.push_back(root->data);
        name->binary_code.push_back(str);
        name->frequency.push_back(root->freq);
    }
    printCodes(root->left, str + "0", name);
    printCodes(root->right, str + "1", name);
}

// build a huffman tree (help from geeksforgeeks)
void HuffmanCodes(std::string data[], int freq[], int size, struct nodeData *name)
{
    struct MinHeapNode *left, *right, *top;

    // Create a min heap & inserts all characters of data[]
    std::priority_queue<MinHeapNode *, std::vector<MinHeapNode *>, compare> minHeap;
    for (int i = 0; i < size; ++i)
        minHeap.push(new MinHeapNode(data[i], freq[i], 0));

    // Iterate while size of heap doesn't become 1
    int s = 0; // int for sorting between 2 sum node (node without character)
    while (minHeap.size() != 1)
    {
        // Extract the two minimum freq items from min heap
        left = minHeap.top();
        minHeap.pop();
        right = minHeap.top();
        minHeap.pop();
        // Create a new  node with frequency equal to the sum of freq of two nodes above. Make sure this node will
        // not be read by add a long string to it data (compare to node with only one charater)
        top = new MinHeapNode("sum", left->freq + right->freq, s);
        top->left = left;
        top->right = right;
        minHeap.push(top);
        s += 1;
    }

    // Print Huffman codes using
    // the Huffman tree built above
    printCodes(minHeap.top(), "", name);
}

void *pthreadFinalLine(void *void_ptr)
{
    struct dataForPthread vdata = *((struct dataForPthread *)void_ptr);
    // critical section 1
    pthread_mutex_lock(vdata.bsem);
    while (*vdata.threadNumber != *vdata.turn)
        pthread_cond_wait(vdata.waitTurn, vdata.bsem);
    pthread_mutex_unlock(vdata.parentThread); // unlock the parent thread
    // store variable to local function
    std::string *compressedLine = &vdata.compressed_line;
    std::string *originalMessage = vdata.originalMessage;
    std::vector<std::string> *pthread_character = &vdata.pthread_character;
    std::vector<std::string> *pthread_binary_code = &vdata.pthread_binary_code;
    std::vector<int> *pthread_frequency = &vdata.pthread_frequency;
    pthread_mutex_unlock(vdata.bsem); // unlock the bsem thread end critical section 1

    std::istringstream ss(*compressedLine);
    std::string word; // substring between 2 space
    std::vector<std::string> splitLine;
    while (ss >> word)
    {
        splitLine.push_back(word);
    }
    for (int i = 0; i < (*pthread_binary_code).size(); i++)
    {
        if (splitLine[0] == (*pthread_binary_code)[i])
        {
            std::cout << "Symbol: " << (*pthread_character)[i] << ", Frequency: " << (*pthread_frequency)[i] << ", Code: " << (*pthread_binary_code)[i] << std::endl;
            break;
        };
    }
    // start making original message;
    std::string letter_temp = " ";
    for (int i = 0; i < (*pthread_binary_code).size(); i++)
    {
        if (splitLine[0] == (*pthread_binary_code)[i])
        {
            letter_temp = (*pthread_character)[i];
        }
    }

    for (int i = 1; i < splitLine.size(); i++)
    {
        int index_temp = 0;
        index_temp = stoi(splitLine[i]);
        (*originalMessage).replace(index_temp, 1, letter_temp);
    };

    // start critical section 2 with turn condition
    pthread_mutex_lock(vdata.bsem);
    *vdata.turn += 1; // Redefine the turn
    pthread_cond_broadcast(vdata.waitTurn);
    pthread_mutex_unlock(vdata.bsem); // end critical section 2
    return nullptr;
}

int main()
{
    std::string line;
    std::string character;
    int frequency;

    std::vector<std::string> line_of_input;

    // //read input for Moodle
    // while (std::getline(std::cin, line))
    // {
    //     line_of_input.push_back(line);
    // }
    // //end reading for Moodle

    // read input for vscode
    std::ifstream iFile;
    iFile.open("input.txt");
    while (std::getline(iFile, line))
    {
        line_of_input.push_back(line);
    }
    iFile.close();
    // end reading for vscode

    std::vector<std::pair<std::string, int>> character_frequency_vector;
    int numbOfSymbol = stoi(line_of_input[0]);
    for (int i = 1; i <= numbOfSymbol; i++)
    {
        std::string aLine = line_of_input[i];
        int space_position = 0;
        std::string temp = "";
        if (aLine[0] != ' ')
        {
            space_position = aLine.find(" ");
            character = aLine.substr(0, space_position);
        }
        else
        {
            space_position = aLine.find(" ") + 1;
            character = " ";
        }
        frequency = stoi(aLine.substr(space_position + 1));
        character_frequency_vector.push_back(make_pair(character, frequency));
    }

    int size = character_frequency_vector.size();
    std::string no_sorting_character[size];
    int no_sorting_freq[size];
    for (int i = 0; i < size; i++)
    {
        no_sorting_character[i] = character_frequency_vector[i].first;
        no_sorting_freq[i] = character_frequency_vector[i].second;
    }

    //  build huffman tree and store it data in a struct
    struct nodeData node;
    // struct nodeData *ptr = &node;
    HuffmanCodes(no_sorting_character, no_sorting_freq, size, &node);

    // input for pthread
    std::string lineOfCompress[size];
    int temp1 = 0;
    for (int i = numbOfSymbol + 1; i < line_of_input.size(); i++)
    {
        lineOfCompress[temp1] = line_of_input[i];
        temp1 += 1;
    }

    // function to decide length of the original message
    int length = 0;
    original_message_length(node.frequency, &length);
    std::string originalMessage(length - 1, '&');

    // start pthread function
    pthread_mutex_t bsem;
    pthread_mutex_t parentThread;
    pthread_cond_t waitTurn = PTHREAD_COND_INITIALIZER;
    pthread_t *tid1 = new pthread_t[size];
    struct dataForPthread vdata;

    // store mutex data to struct properties
    vdata.bsem = &bsem;
    vdata.parentThread = &parentThread;
    vdata.waitTurn = &waitTurn;
    vdata.pthread_character = node.character;
    vdata.pthread_binary_code = node.binary_code;
    vdata.pthread_frequency = node.frequency;

    // start mutex
    pthread_mutex_init(&bsem, NULL);
    pthread_mutex_init(&parentThread, NULL);
    int tNumber = -1;
    int turn = 0;
    for (int i = 0; i < size; i++)
    {
        vdata.originalMessage = &originalMessage;
        vdata.turn = &turn;
        vdata.threadNumber = &tNumber;
        pthread_mutex_lock(&parentThread);
        vdata.compressed_line = lineOfCompress[i];
        tNumber++;
        if (pthread_create(&tid1[i], nullptr, pthreadFinalLine, &vdata))
        {
            std::cerr << "Error creating thread \n";
            return 1;
        }
    }
    for (int i = 0; i < size; i++)
        pthread_join(tid1[i], nullptr);

    std::cout << "Original message: " << originalMessage << std::endl;
    return 0;
}