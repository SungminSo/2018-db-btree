USE Pokemon;

#1
SELECT name FROM Trainer as T,CatchedPokemon as C WHERE T.id = owner_id GROUP BY owner_id HAVING COUNT(pid)>=3 ORDER BY COUNT(pid) DESC,T.name;
#2
SELECT name FROM Pokemon,(SELECT type FROM Pokemon GROUP BY type ORDER BY COUNT(id) DESC,type ASC LIMIT 2) as P WHERE Pokemon.type = P.type ORDER BY name ASC;
#3
SELECT name FROM Pokemon WHERE name LIKE '_o%' ORDER BY name ASC; 
#4
SELECT nickname FROM CatchedPokemon WHERE level >= 50 ORDER BY nickname ASC;
#5
SELECT name FROM Pokemon WHERE LENGTH(name) = 6 ORDER BY name ASC;
#6
SELECT name FROM Trainer WHERE hometown = 'Blue City' ORDER BY name ASC;
#7
SELECT DISTINCT hometown FROM Trainer ORDER BY hometown ASC;
#8
SELECT AVG(level) FROM CatchedPokemon as C WHERE C.owner_id IN(SELECT id FROM Trainer as T WHERE T.name = 'Red');
#9
SELECT nickname FROM CatchedPokemon WHERE nickname NOT LIKE 'T%' ORDER BY nickname ASC;
#10
SELECT nickname FROM CatchedPokemon WHERE level >= 50 and owner_id >= 6 ORDER BY nickname ASC;
#11
SELECT id, name FROM Pokemon ORDER BY id ASC;
#12
SELECT nickname FROM CatchedPokemon WHERE level <= 50 ORDER BY level ASC;
#13
SELECT name,pid FROM CatchedPokemon as C, Pokemon as P WHERE C.pid = P.id and owner_id IN(SELECT id FROM Trainer WHERE hometown='Sangnok City') ORDER BY pid ASC;
#14
SELECT nickname FROM CatchedPokemon as C, Pokemon as P WHERE C.pid = P.id and P.type = 'Water' and C.owner_id IN (SELECT leader_id FROM Gym) ORDER BY nickname;
#15
SELECT count(*) FROM CatchedPokemon where pid IN (SELECT before_id FROM Evolution);
#16
SELECT COUNT(*) FROM Pokemon WHERE type='Water' or type='Electric' or type='Psychic';
#17
SELECT COUNT(DISTINCT pid) as count FROM CatchedPokemon WHERE owner_id IN(SELECT id FROM Trainer WHERE hometown='Sangnok City');
#18
SELECT MAX(level) FROM CatchedPokemon WHERE owner_id IN(SELECT id FROM Trainer WHERE hometown='Sangnok City');
#19
SELECT COUNT(DISTINCT type) FROM Pokemon as P,CatchedPokemon as C WHERE P.id=C.pid and C.owner_id IN(SELECT leader_id FROM Gym WHERE city='Sangnok City');
#20
SELECT T.name, count(pid) FROM Trainer as T,CatchedPokemon as C WHERE T.id=C.owner_id and T.hometown='Sangnok City' GROUP BY C.owner_id ORDER BY COUNT(pid);
#21
SELECT name FROM Pokemon WHERE name LIKE 'A%' or name LIKE 'E%' or name LIKE 'U%' or name LIKE 'I%' or name LIKE 'O%';
#22
SELECT type,count(id) from Pokemon GROUP BY type ORDER BY count(id) ASC,type ASC;
#23
SELECT DISTINCT name FROM Trainer as T,CatchedPokemon as C WHERE level <= 10 and T.id=C.owner_id ORDER BY name ASC;
#24
SELECT CI.name,AVG(level) FROM City as CI,Trainer as T,CatchedPokemon as C WHERE CI.name=T.hometown and T.id=C.owner_id GROUP BY hometown ORDER BY AVG(level) ASC;
#25
SELECT DISTINCT P.name FROM Pokemon as P,(SELECT pid FROM CatchedPokemon as C,Trainer as T WHERE T.id=C.owner_id and T.hometown='Sangnok City') as S,(SELECT pid FROM CatchedPokemon as C,Trainer as T WHERE T.id=C.owner_id and T.hometown='Brown City') as B WHERE S.pid=B.pid and S.pid=P.id ORDER BY P.name ASC;
#26
SELECT P.name FROM CatchedPokemon as C, Pokemon as P WHERE C.pid=P.id and C.nickname LIKE '% %' ORDER BY name DESC;
#27
SELECT T.name,MAX(C.level) FROM Trainer as T, CatchedPokemon as C WHERE T.id=C.owner_id and T.id IN(SELECT owner_id FROM CatchedPokemon as C GROUP BY owner_id HAVING COUNT(pid)>=4) GROUP BY T.name HAVING MAX(C.level) ORDER BY T.name ASC;
#28
SELECT T.name,avg(C.level) FROM Trainer as T, CatchedPokemon as C,Pokemon as P WHERE C.owner_id=T.id and C.pid=P.id and (P.type='Normal' or P.type='Electric') GROUP BY T.name ORDER BY AVG(C.level) ASC;
#29
SELECT P.name,T.name,description FROM Pokemon as P, CatchedPokemon as C, Trainer as T, City as CI WHERE P.id=152 and C.pid=P.id and T.id=C.owner_id and T.hometown = CI.name ORDER BY C.level DESC;
#30
SELECT P1.id, P1.name, P2.name, P3.name FROM Pokemon as P1, Pokemon as P2, Pokemon as P3, Evolution as E1, Evolution as E2 WHERE E1.after_id = E2.before_id and P1.id = E1.before_id and P2.id = E1.after_id and P3.id = E2.after_id;
#31
SELECT name FROM Pokemon as P WHERE P.id > 9 and P.id < 100 ORDER BY name ASC;
#32
SELECT name FROM Pokemon as P WHERE NOT EXISTS(SELECT * FROM CatchedPokemon as C WHERE P.id = C.pid) ORDER BY name ASC;
#33
SELECT SUM(level) FROM Trainer as T,CatchedPokemon as C WHERE T.name='Matis' and C.owner_id = T.id;
#34
SELECT name FROM Gym,Trainer where Gym.leader_id = Trainer.id ORDER BY name ASC;
#35
SELECT type,(COUNT(type)*100/(SELECT COUNT(*) FROM Pokemon)) as Percent FROM Pokemon GROUP BY type ORDER BY Percent DESC LIMIT 1;
#36
SELECT T.name FROM Trainer as T, CatchedPokemon as C WHERE C.owner_id=T.id and C.nickname LIKE 'A%' ORDER BY T.name ASC;
#37
SELECT name,SUM(level) FROM Trainer as T,CatchedPokemon as C WHERE T.id=C.owner_id group by owner_id ORDER BY SUM(level) DESC LIMIT 1;
#38
SELECT P.name FROM Pokemon as P, Evolution as E WHERE P.id = E.after_id and E.after_id NOT IN (select E2.after_id from Evolution as E1,Evolution as E2 where E1.after_id=E2.before_id) ORDER BY P.name ASC;
#39
SELECT T.name FROM Trainer as T,CatchedPokemon as C WHERE T.id=C.owner_id GROUP BY C.owner_id HAVING COUNT(pid)-COUNT(DISTINCT pid)>=1 ORDER BY T.name ASC;
#40
SELECT * from (SELECT T.hometown,C.nickname FROM Trainer as T,CatchedPokemon as C WHERE T.id=C.owner_id order by C.level DESC) as X GROUP BY X.hometown ORDER BY X.hometown ASC;
