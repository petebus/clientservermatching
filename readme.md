# Client-server exchange

It's a console simple matching client-server engine which have a connect to PostgreSQL.

### Build instructions

Require C++20.

 1. Install Cmake
 2. Install PostgreSQL 13.9
 3. Install Boost 1.81
 4. Import dump.sql into your database via next command:
>     psql -h hostname -d databasename -U username -f dump.sql

5. Configure **connectionString** settings in Common.hpp
6. Run **Server** and then **Client** projects

---

### How it works?

When you enter the program, you will be prompted with an authorization form. If you are already registered, select the first option and log in.
Next, you'll see a menu options list:
1. Add order
2. Delete order
3. Show balance
4. Show order list
5. Show quotes
6. Exit

The order adding format looks: "BUY/SELL volume (in USD) price (in RUB)".

For example:
> **BUY 1000 69**

> **SELL 1500 73**

The order is deleted by 'order_id' from orders table. 

While we matching a deal: orders volume decreasing by deal volume, next change the status of orders (**'active'** -> **'completed'** if order had declined volume to zero) and finally change users balances.

---
### SQL tables view
#### users

| id  | username | password | balance_usd | balance_rub | active_orders |
| ------------- | ------------- | ------------- | ------------- | ------------- | ------------- | 
| 1  | Name  | ********** | 5000 | 200000 | {1,5,31,54,62}
| ...  | ... | ... | ... | ... | ... |

#### orders

| type  | value | price | user_id | time | status | order_id |
| ------------- | ------------- | ------------- | ------------- | ------------- | ------------- | ------------- | 
| BUY  | 100  | 70 | 1 | 1970-01-01 00:10:00 | active | 31
| SELL  | 500  | 70 | 2 | 1970-01-01 00:00:00 | completed | 2
| ...  | ... | ... | ... | ... | ... | ... |

#### matches

| buyer  | seller | value | price | 
| ------------- | ------------- | ------------- | ------------- |
| 1  | 2  | 1500 | 70 |
| ...  | ... | ... | ... |

---

### Components
* https://github.com/jtv/libpqxx
* https://www.boost.org
* https://github.com/nlohmann/json
* https://www.postgresql.org/download/