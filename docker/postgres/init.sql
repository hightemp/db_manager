-- Create departments table
CREATE TABLE departments (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    location VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create employees table
CREATE TABLE employees (
    id SERIAL PRIMARY KEY,
    department_id INTEGER,
    first_name VARCHAR(50) NOT NULL,
    last_name VARCHAR(50) NOT NULL,
    email VARCHAR(100) UNIQUE,
    salary DECIMAL(10,2),
    hire_date DATE,
    FOREIGN KEY (department_id) REFERENCES departments(id)
);

-- Insert test data into departments
INSERT INTO departments (name, location) VALUES
('Engineering', 'Building A'),
('Marketing', 'Building B'),
('Sales', 'Building C'),
('Human Resources', 'Building A');

-- Insert test data into employees
INSERT INTO employees (department_id, first_name, last_name, email, salary, hire_date) VALUES
(1, 'John', 'Smith', 'john.smith@example.com', 75000.00, '2020-01-15'),
(1, 'Jane', 'Doe', 'jane.doe@example.com', 82000.00, '2019-03-20'),
(2, 'Mike', 'Johnson', 'mike.j@example.com', 65000.00, '2021-02-10'),
(2, 'Sarah', 'Williams', 'sarah.w@example.com', 67000.00, '2020-11-05'),
(3, 'Robert', 'Brown', 'robert.b@example.com', 71000.00, '2018-07-22'),
(3, 'Lisa', 'Anderson', 'lisa.a@example.com', 69000.00, '2019-09-15'),
(4, 'David', 'Wilson', 'david.w@example.com', 62000.00, '2021-04-30'),
(1, 'Emma', 'Taylor', 'emma.t@example.com', 78000.00, '2020-08-12'),
(2, 'James', 'Martin', 'james.m@example.com', 66000.00, '2021-01-25'),
(4, 'Anna', 'Clark', 'anna.c@example.com', 63000.00, '2020-06-18'); 