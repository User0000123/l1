#include <gsl/gsl_vector.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "DArray.h"
#include "Parser.h"

void InitializeObjFileInfo(ObjFile *pObjFile)
{
    pObjFile->v =   CreateArray(2);
    pObjFile->vt =  CreateArray(2);
    pObjFile->vn =  CreateArray(2);
    pObjFile->vp =  CreateArray(2);
    pObjFile->fv =  CreateArray(2);
    pObjFile->fvt = CreateArray(2);
    pObjFile->fvn = CreateArray(2);
    pObjFile->l =   CreateArray(2);
}

void DestroyObjFileInfo(ObjFile *pObjFile)
{
    DestroyArray(pObjFile->v);
    DestroyArray(pObjFile->vt);
    DestroyArray(pObjFile->vn);
    DestroyArray(pObjFile->vp);
    DestroyArray(pObjFile->fv);
    DestroyArray(pObjFile->fvt);
    DestroyArray(pObjFile->fvn);
    DestroyArray(pObjFile->l);
}

void TrimObjFileArrays(ObjFile *pObjFile)
{
    TrimToSize(pObjFile->v);
    TrimToSize(pObjFile->vt);
    TrimToSize(pObjFile->vn);
    TrimToSize(pObjFile->vp);
    TrimToSize(pObjFile->fv);
    TrimToSize(pObjFile->fvt);
    TrimToSize(pObjFile->fvn);
    TrimToSize(pObjFile->l);
}

/* Parce vertex from string */
void* vParce(char *string)
{
    _CRT_DOUBLE dbval;
    gsl_vector *pVector = gsl_vector_alloc(4);
    char *token;
    int index = 0;

    // Use default value
    pVector->data[3] = 1.0;

    while ((token = strtok(NULL, PARSER_COMPONENTS_DELIMETER)) != NULL)
    {
        _atodbl(&dbval, token); 
        pVector->data[index++] = dbval.x;
    } 

    return pVector;
}

/* Parce texture/normals/space vertex from string */
void* vSParce(char *string)
{
    _CRT_DOUBLE dbval;
    gsl_vector *pVector = gsl_vector_calloc(4);
    char *token;
    int index = 0;

    while ((token = strtok(NULL, PARSER_COMPONENTS_DELIMETER)) != NULL)
    {
        _atodbl(&dbval, token); 
        pVector->data[index++] = dbval.x;
    } 

    return pVector;
}

int getTokensCount(char *string, char *delimeter)
{
    int dim = 1;
    char tempString[PARSER_MAX_STRING_LENGTH];

    strcpy(tempString, string);
    strtok(tempString, delimeter);

    while (strtok(NULL, delimeter) != NULL)
    {
        dim++;
    }

    return dim;
}

/* Parce polygon from string */
void parcePolygon(ObjFile *pObjFile, char *string)
{
    int dimensionality = getTokensCount(string, PARSER_COMPONENTS_DELIMETER);
    gsl_vector_int *pVectorVerteces = gsl_vector_int_calloc(dimensionality);
    gsl_vector_int *pVectorTextures = gsl_vector_int_calloc(dimensionality);
    gsl_vector_int *pVectorNormals = gsl_vector_int_calloc(dimensionality);

    int index = 0;
    char *token;
    char **components = calloc(dimensionality, sizeof(char *));

    token = strtok(string, PARSER_COMPONENTS_DELIMETER);
    
    while (token != NULL)
    {
        components[index++] = token;        

        token = strtok(NULL, PARSER_COMPONENTS_DELIMETER);
    } 

    int nCoords = getTokensCount(components[0], PARSER_COORDINATES_DELIMETER);
    int isVtExists = 0;
    int isVnExists = 0;

    token = strtok(components[0], PARSER_COORDINATES_DELIMETER);
    pVectorVerteces->data[0] = atoi(token);

    token = strtok(NULL, PARSER_COORDINATES_DELIMETER);
    isVtExists = strnlen_s(token, PARSER_MAX_STRING_LENGTH) != 0;

    if (isVtExists)
    { 
        pVectorTextures->data[0] = atoi(token);
    } 
    
    if (token = strtok(NULL, PARSER_COORDINATES_DELIMETER))
    {
        
        pVectorNormals->data[0] = atoi(token);
        isVnExists = 1;
    }

    for (int i = 1; i < dimensionality; i++)
    {
        token = strtok(components[i], PARSER_COORDINATES_DELIMETER);
        pVectorVerteces->data[i] = atoi(token);

        if (isVtExists)
        {
            token = strtok(NULL, PARSER_COORDINATES_DELIMETER);
            pVectorTextures->data[i] = atoi(token);
        } 
        else
        {
            strtok(NULL, PARSER_COORDINATES_DELIMETER);
        }

        if (isVnExists)
        {
            token = strtok(NULL, PARSER_COORDINATES_DELIMETER);
            pVectorNormals->data[i] = atoi(token);;
        }
    }

    int k = dimensionality;
    while (k > 2)
    {
        gsl_vector_int *temp = gsl_vector_int_calloc(3);
        temp->data[0] = pVectorVerteces->data[0];
        temp->data[1] = pVectorVerteces->data[k-2];
        temp->data[2] = pVectorVerteces->data[k-1];

        k--;
        Add(pObjFile->fv, temp);
    }
    // Add(pObjFile->fv, pVectorVerteces);
    
    if (isVtExists)
    {
        k = dimensionality;
        while (k > 2)
        {
            gsl_vector_int *temp = gsl_vector_int_calloc(3);
            temp->data[0] = pVectorTextures->data[0];
            temp->data[1] = pVectorTextures->data[k-2];
            temp->data[2] = pVectorTextures->data[k-1];

            k--;
            Add(pObjFile->fvt, temp);
        }
        // Add(pObjFile->fvt, pVectorTextures);
    }
    else
    {
        gsl_vector_int_free(pVectorTextures);
    }

    if (isVnExists)
    {
        k = dimensionality;
        while (k > 2)
        {
            gsl_vector_int *temp = gsl_vector_int_calloc(3);
            temp->data[0] = pVectorNormals->data[0];
            temp->data[1] = pVectorNormals->data[k-2];
            temp->data[2] = pVectorNormals->data[k-1];

            k--;
            Add(pObjFile->fvn, temp);
        }
        // Add(pObjFile->fvn, pVectorNormals);
    }
    else
    {
        gsl_vector_int_free(pVectorNormals);
    }

    free(components);
}

ObjFile *parseOBJ(FILE *stream)
{
    ObjFile *pObjFile = malloc(sizeof(ObjFile));
    InitializeObjFileInfo(pObjFile);

    char string[PARSER_MAX_STRING_LENGTH];
    char *token, *ts;

    while (!feof(stream))
    {
        fgets(string, PARSER_MAX_STRING_LENGTH, stream);
        token = strtok(string, PARSER_COMPONENTS_DELIMETER);

        ts = token + strlen(token) + 1;
        while(isspace((unsigned char)*ts)) ts++;
        char *end = ts + strlen(ts) - 1;
        while(end > ts && isspace((unsigned char)*end)) 
        {
            *end = 0;
            end--;
        }

        if (strcmp(token, "v") == 0)
        {   
            Add(pObjFile->v, vParce(ts));
        }
        else if (strcmp(token, "vn") == 0)
        {
            Add(pObjFile->vn, vSParce(ts));
        }
        else if (strcmp(token, "vt") == 0)
        {
            Add(pObjFile->vt, vSParce(ts));
        }
        else if (strcmp(token, "vp") == 0)
        {
            Add(pObjFile->vp, vSParce(ts));
        }
        else if (strcmp(token, "f") == 0)
        {
            parcePolygon(pObjFile, ts);
        }
    }

    TrimObjFileArrays(pObjFile);
    return pObjFile;   
}

// int main()
// {
//     FILE *objFile;
//     char* fileName = "C:\\Users\\Aleksej\\Downloads\\model\\model.obj";
//     // char* fileName = "C:\\Users\\Aleksej\\Desktop\\labs\\ACG\\l1\\t.txt";

//     objFile = fopen(fileName, "r");

//     DestroyObjFileInfo(parseOBJ(objFile));

//     fclose(objFile);
//     return 0;
// }