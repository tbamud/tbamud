The Message Editor (BETA):
  tbaMUD 3.64

WARNING: THE NEW MESSAGE EDITOR WILL ALTER THE EXISTING MESSAGE FILE BY CONVERTING
IT TO A MORE EASILY MACHINE READABLE FORMAT WHEN A MESSAGE IS SUCCESSFULLY EDITED.

The Message Editor is an OLC style editor for modifying the in-game combat messages.

Simply typing "msgedit" will bring up a list of all active in-game combat messages,
along with the skill they are associated with, number of entries, and a name (if
one exists.)

========================================

Message List: 
0 ) [5  ] 1, burning hands        30) [314] 2, Unknown           
1 ) [6  ] 1, call lightning       31) [399] 2, Unknown           
2 ) [8  ] 1, chill touch          32) [33 ] 1, poison            
3 ) [10 ] 1, color spray          33) [202] 1, !UNUSED!          
4 ) [22 ] 1, dispel evil          34) [203] 1, !UNUSED!          
5 ) [23 ] 1, earthquake           35) [204] 1, !UNUSED!          
6 ) [25 ] 1, energy drain         36) [205] 1, !UNUSED!          
7 ) [26 ] 1, fireball             37) [206] 1, !UNUSED!      

=========================================

MSG #, SKILL NUMBER, NUMBER OF ENTRIES, NAME (if applicable)
0 )    [5  ]         1,                 burning hands


To edit a message, use the command "msgedit" along with the entry # of the message
you would like to edit. To edit the "burning hands" entry, the correct command would
be:

  > msgedit 0

==========================================

Msg Edit: [0x1] [$n: Attacker | $N: Victim]
1) Action Type: 5 [burning hands]
   Death Messages:
A) CHAR :  You have burned $N to death!
B) VICT :  You have been burned to death by $n!
C) ROOM :  $n has burned $N to death!
   Miss Messages:
D) CHAR :  You miss $N with your burning hands.
E) VICT :  $n reaches for you with burning hands, but misses!
F) ROOM :  $n tries to burn $N, but $E escapes!
   Hit Messages:
G) CHAR :  You burn $N with your hot little hands!
H) VICT :  You cry out in pain as $n grabs you with burning hands!
I) ROOM :  $N cries out as $n burns $M!
   God Messages:
J) CHAR :  Your attempt to burn $N nears BLASPHEMY!!!
K) VICT :  The pitiful creature before you tries to burn you.
L) ROOM :  Unaware of the risks, $n tries to burn $N.

N) New Q) Quit

============================================

To edit a message, simply enter the applicable letter for the line to which you'd
like to edit, and an entry prompt will emerge along with an example of use.

To edit the A), "char death message", simply type from the message editor:
  > a

============================================

Enter Selection : a
Example: You kill $N!
Enter new string : 

=============================================

Developed by Joseph Arnusch (Vatiken) for use in tbaMUD 3.64.
