# etalk
A Simple Object-Oriented Language

The program starts in a function named "MAIN".

```
program:
    <definition>*

definition:
    <class-definition>
|   <function-definition>

class-definition:
    CLASS <name> [ : <super-class-name> ]
      <class-statement>*
    End Class

class-statement:
    IVARS <id> [ , <id> ]*
|   CVARS <id> [ , <id> ]*
|   <method-definition>
|   <cmethod-definition>

method-definition:
    METHOD <name> ( [ <arg> [ , <arg> ]* ] )
      <statement>*
    END METHOD

cmethod-definition:
    CMETHOD <name> ( [ <arg> [ , <arg> ]* ] )
      <statement>*
    END CMETHOD

function-definition:
    FUNCTION <name> ( [ <arg> [ , <arg> ]* ] )
      <statement>*
    END FUNCTION

statement:
    <if-statement>
|   <while-statement>
|   <return-statement>
|   <expression> ;

if-statement:
    IF <expression>
    THEN
      <statement>*
  [ ELSE
      <statement>* ]
    END IF

while-statement:
    WHILE <expression> DO
      <statement>*
    END WHILE

return-statement:
    RETURN <expression>

expression:
    <expression> + <expression>
|   <expression> - <expression>
|   <expression> * <expression>
|   <expression> < <expression>
|   <expression> = <expression>
|   <expression> > <expression>
|   <variable> := <expression>
|   <expression> ( [ <expression> [ , <expression> ]* ] )
|   [ <selector-and-arguments> ]
|   ( <expression> )
|   <variable>
|   <integer-constant>
|   <string-constant>

selector-and-arguments:
    (how to describe this?)
```
