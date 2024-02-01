#include <gsl/gsl_matrix.h>
#include <stdlib.h>
#include <string.h>

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
    gsl_vector *pVector = gsl_vector_calloc(3);
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
    _CRT_DOUBLE dbval;
    gsl_vector *pVectorVerteces = gsl_vector_calloc(dimensionality);
    gsl_vector *pVectorTextures = gsl_vector_calloc(dimensionality);
    gsl_vector *pVectorNormals = gsl_vector_calloc(dimensionality);

    int index = 0;
    char *token;
    char **components = calloc(dimensionality, sizeof(char *));

    token = strtok(string, PARSER_COMPONENTS_DELIMETER);
    
    while (token != NULL)
    {
        components[index++] = token;
        // printf("TOKEN:%s \n", token);

        token = strtok(NULL, PARSER_COMPONENTS_DELIMETER);
    } 

    int nCoords = getTokensCount(components[0], PARSER_COORDINATES_DELIMETER);
    int isVtExists = 0;
    int isVnExists = 0;

    token = strtok(components[0], PARSER_COORDINATES_DELIMETER);
    _atodbl(&dbval, token);
    pVectorVerteces->data[0] = dbval.x;

    token = strtok(NULL, PARSER_COORDINATES_DELIMETER);
    isVtExists = strlen(token) != 0;

    if (isVtExists)
    {
        _atodbl(&dbval, token);
        pVectorTextures->data[0] = dbval.x;
    } 
    
    if (token = strtok(NULL, PARSER_COORDINATES_DELIMETER))
    {
        _atodbl(&dbval, token);
        pVectorNormals->data[0] = dbval.x;
        isVnExists = 1;
    }

    for (int i = 1; i < dimensionality; i++)
    {
        token = strtok(components[i], PARSER_COORDINATES_DELIMETER);
        _atodbl(&dbval, token);
        pVectorVerteces->data[i] = dbval.x;

        if (isVtExists)
        {
            token = strtok(NULL, PARSER_COORDINATES_DELIMETER);
            _atodbl(&dbval, token);
            pVectorTextures->data[i] = dbval.x;
        } 
        else
        {
            strtok(NULL, PARSER_COORDINATES_DELIMETER);
        }

        if (isVnExists)
        {
            token = strtok(NULL, PARSER_COORDINATES_DELIMETER);
            _atodbl(&dbval, token);
            pVectorNormals->data[i] = dbval.x;
        }
    }

    Add(pObjFile->fv, pVectorVerteces);
    
    if (isVtExists)
    {
        Add(pObjFile->fvt, pVectorTextures);
    }
    else
    {
        gsl_vector_free(pVectorTextures);
    }

    if (isVnExists)
    {
        Add(pObjFile->fvn, pVectorNormals);
    }
    else
    {
        gsl_vector_free(pVectorNormals);
    }

    free(components);
}

ObjFile *parceOBJ(FILE *stream)
{
    ObjFile *pObjFile = malloc(sizeof(ObjFile));
    InitializeObjFileInfo(pObjFile);

    char string[PARSER_MAX_STRING_LENGTH];
    char *token;

    while (!feof(stream))
    {
        fgets(string, PARSER_MAX_STRING_LENGTH, stream);
        token = strtok(string, PARSER_COMPONENTS_DELIMETER);

        if (strcmp(token, "v") == 0)
        {   
            Add(pObjFile->v, vParce(token + strlen(token) + 1));
        }
        else if (strcmp(token, "vn") == 0)
        {
            Add(pObjFile->vn, vSParce(token + strlen(token) + 1));
        }
        else if (strcmp(token, "vt") == 0)
        {
            Add(pObjFile->vt, vSParce(token + strlen(token) + 1));
        }
        else if (strcmp(token, "vp") == 0)
        {
            Add(pObjFile->vp, vSParce(token + strlen(token) + 1));
        }
        else if (strcmp(token, "f") == 0)
        {
            parcePolygon(pObjFile, token + strlen(token) + 1);
        }
    }

    TrimObjFileArrays(pObjFile);
    return pObjFile;   
}

int main()
{
    FILE *objFile;
    char* fileName = "C:\\Users\\Aleksej\\Downloads\\model\\model.obj";
    // char* fileName = "C:\\Users\\Aleksej\\Desktop\\labs\\ACG\\l1\\t.txt";

    objFile = fopen(fileName, "r");

    DestroyObjFileInfo(parceOBJ(objFile));

    fclose(objFile);
    return 0;
}