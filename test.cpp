#include<iostream>
#include<vector>
#include<sstream>
#include <bitset>
#include <algorithm>
#include<math.h>
using namespace std;

string findTwoscomplement(string str)
{
    int n = str.length();
 
    // Traverse the string to get first '1' from
    // the last of string
    int i;
    for (i = n-1 ; i >= 0 ; i--)
        if (str[i] == '1')
            break;
 
    // If there exists no '1' concatenate 1 at the
    // starting of string
    if (i == -1)
        return '1' + str;
 
    // Continue traversal after the position of
    // first '1'
    for (int k = i-1 ; k >= 0; k--)
    {
        //Just flip the values
        if (str[k] == '1')
            str[k] = '0';
        else
            str[k] = '1';
    }
 
    // return the modified string
    return str;;
}

vector<int> ToBinary(unsigned long long num)
{
    vector<int> bit_vector;
    string str="";
    while (num >  0)
    {
        bit_vector.push_back(num%2);
        num = num/2;
    }reverse(bit_vector.begin(), bit_vector.end());
    return bit_vector;
}

signed long long TwoElement(unsigned long long num){

    // Find binary vector for number
    vector<int> bit_vector;

    while (num >  0)
    {
        bit_vector.push_back(num%2);
        num = num/2;
    }reverse(bit_vector.begin(), bit_vector.end());

    // substract -1 from binary
    bool borrow_bit = 0;
    if (bit_vector.back() == 0){
        borrow_bit =1;
        bit_vector.back() = 1;
    }else{
        bit_vector.back() = 0;
    }

    int i = bit_vector.size() - 2;

    while (borrow_bit != 0)
    {
        if(bit_vector[i] == 0){
            borrow_bit =1;
            bit_vector[i] = 1;
            i = i-1;
        }else{
            borrow_bit = 0;
            bit_vector[i] = 0;
        }
    }

    cout << endl;
    for(int j : bit_vector){
        cout << j;
    }
    cout << endl;

    // XOR
    vector<int> vt;
    for(int j = 0; j<bit_vector.size(); j++){
        if(bit_vector[j] == 1) vt.push_back(0);
        else vt.push_back(1);
    }

    //cout << "size: " << vt.size() << endl;
    for(int k : vt){
        cout << k;
    }
    cout << endl;

    signed long long result = 0;
    int m = 0;
    for(int i=vt.size()-1; i>=0; i--){
        result += vt[i] * pow(2, m);
        m++;
    }

    return -1*result;
    
}



int main()
{
    vector<int> vt = ToBinary(4285525909);
    //vt.push_back(3);
    // for(int i : vt){
    //     cout << i;
    // }
    //cout << endl << vt.back();

    cout << TwoElement(4285525909);
  
    return 0;
}