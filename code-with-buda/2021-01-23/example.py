import csv
import mgclient

conn = mgclient.connect(host='127.0.0.1', port=7687)
conn.autocommit = True

cursor = conn.cursor()
cursor.execute('MATCH (n) DETACH DELETE n;')
cursor.execute('CREATE INDEX ON :Movie(id);')
cursor.execute('CREATE INDEX ON :User(id);')
with open('ml-latest-small/movies.csv', 'r') as f:
    reader = csv.DictReader(f)
    for row in reader:
        try:
            cursor.execute(
                'CREATE (:Movie {id: "%s", title: "%s"});' %
                (row['movieId'], row['title']))
        except Exception:
            pass
with open('ml-latest-small/ratings.csv', 'r') as f:
    reader = csv.DictReader(f)
    user_ids = set()
    for row in reader:
        if not row['userId'] in user_ids:
            cursor.execute('CREATE (:User {id: "%s"});' % row['userId'])
        user_ids.add(row['userId'])
        query = 'MATCH (m:Movie {id: "%s"}), (u:User {id: "%s"}) CREATE (u)-[:RATE {rating: %f}]->(m);' % (
            row['movieId'], row['userId'], float(row['rating']))
        cursor.execute(query)
