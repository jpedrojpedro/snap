-- test_pipeline.lua
print("--- Starting Monolith Verification Script ---")

-- 1. Create dummy mock data via SQL query
print("[Test 1] Initializing mock data file...")
snap.query([[
    COPY (
        SELECT 1 as id, 'Alice' as name, 'Data Engineer' as role, 10500.00 as salary
        UNION ALL
        SELECT 2 as id, 'Bob' as name, 'Architect' as role, 12000.00 as salary
        UNION ALL
        SELECT 3 as id, 'Charlie' as name, 'Analyst' as role, 8500.00 as salary
    ) TO 'tests/mock_staff.parquet' (FORMAT 'PARQUET');
]])

-- 2. Validate auto-inference load capability
print("[Test 2] Testing auto-inference loader...")
snap.read("tests/mock_staff.parquet")

-- 3. Assert schema evaluation capabilities
print("[Test 3] Printing database schema metadata:")
snap.meta()

-- 4. Assert Analytical Data Transformation & Markdown output formatting
print("[Test 4] Executing analytical aggregation query:")
local query_str = [[
    SELECT 
        role, 
        COUNT(*) as total_employees, 
        ROUND(AVG(salary), 2) as average_salary 
    FROM mock_staff 
    GROUP BY role 
    ORDER BY average_salary DESC;
]]
snap.query(query_str, "markdown")

-- 5. Test statistical profile command
print("[Test 5] Running automated target profiling:")
snap.profile("mock_staff")

-- 6. Verify export facilities
print("[Test 6] Testing pipeline data exporting capabilities...")
snap.export("tests/high_earners.csv", "SELECT * FROM mock_staff WHERE salary > 9000")

print("--- Integration Tests Successfully Completed ---")
